#!/usr/bin/env ruby

require 'fileutils'

# Путь к директории с исходными файлами
src_dir = "src"
output_file = "project_diagram.md"

# Получаем все исходные файлы .cpp и .cppm из директории src
sources = Dir.glob("#{src_dir}/**/*.{cpp,cppm,h,hpp}")

# Структуры для хранения информации о классах и их связях
classes = {}
relationships = []
modules = {}

# Регулярные выражения для поиска классов, наследования и зависимостей
class_regex = /\bclass\s+(\w+)(?:\s*:\s*(?:public|protected|private)\s+(\w+))?/
module_regex = /^\s*export\s+module\s+([\w\.]+);/
import_regex = /^\s*import\s+([\w\.]+);/
member_regex = /\b(\w+(?:<[\w\s,]+>)?)\s+(\w+)\s*;/
method_regex = /\b(\w+)\s*\(([^)]*)\)\s*(?:const|override|final|noexcept)?\s*(?:=\s*\w+)?\s*(?:{\s*|\s*;)/

# Анализ исходных файлов
sources.each do |file|
  content = File.read(file)

  # Находим модуль
  module_match = content.match(module_regex)
  current_module = module_match ? module_match[1] : nil

  if current_module
    modules[current_module] = { file: file, imports: [] }

    # Находим импорты
    content.scan(import_regex).each do |import|
      modules[current_module][:imports] << import[0]
    end
  end

  # Находим классы
  content.scan(class_regex).each do |match|
    class_name = match[0]
    parent_class = match[1]

    classes[class_name] ||= {
      module: current_module,
      file: file,
      methods: [],
      members: [],
      parents: []
    }

    if parent_class
      classes[class_name][:parents] << parent_class
      relationships << { from: class_name, to: parent_class, type: "inheritance" }
    end

    # Находим методы класса
    class_content = content[/class\s+#{class_name}.*?(?=\bclass\b|\z)/m] || ""
    class_content.scan(method_regex).each do |method_match|
      method_name = method_match[0]
      params = method_match[1]
      classes[class_name][:methods] << { name: method_name, params: params }
    end

    # Находим члены класса
    class_content.scan(member_regex).each do |member_match|
      type = member_match[0]
      name = member_match[1]
      classes[class_name][:members] << { type: type, name: name }

      # Проверяем, является ли тип другим классом из нашего проекта
      if classes.key?(type) || type.include?("shared_ptr<") || type.include?("unique_ptr<")
        referenced_class = type.gsub(/shared_ptr<|unique_ptr<|>|\s+/, '')
        if classes.key?(referenced_class)
          relationships << { from: class_name, to: referenced_class, type: "association" }
        end
      end
    end
  end
end

# Генерация диаграммы Mermaid
diagram = "# Диаграмма проекта\n\n"
diagram += "```mermaid\nclassDiagram\n"

# Добавляем классы и их методы/члены
classes.each do |class_name, info|
  diagram += "    class #{class_name} {\n"

  # Добавляем члены класса
  info[:members].each do |member|
    diagram += "        +#{member[:type]} #{member[:name]}\n"
  end

  # Добавляем методы класса
  info[:methods].each do |method|
    params_str = method[:params].strip.empty? ? "" : method[:params]
    diagram += "        +#{method[:name]}(#{params_str})\n"
  end

  diagram += "    }\n"
end

# Добавляем связи между классами
relationships.uniq.each do |rel|
  case rel[:type]
  when "inheritance"
    diagram += "    #{rel[:from]} --|> #{rel[:to]}\n"
  when "association"
    diagram += "    #{rel[:from]} --> #{rel[:to]}\n"
  end
end

# Добавляем связи между модулями
modules.each do |mod_name, mod_info|
  mod_info[:imports].each do |import|
    diagram += "    #{mod_name.gsub('.', '_')} ..> #{import.gsub('.', '_')} : imports\n"
  end
end

diagram += "```\n"

# Добавляем легенду
diagram += "\n## Легенда\n\n"
diagram += "- `-->` : Ассоциация (один класс использует другой)\n"
diagram += "- `--|>` : Наследование\n"
diagram += "- `..>` : Зависимость (импорт модуля)\n"

# Записываем диаграмму в файл
File.write(output_file, diagram)
puts "Диаграмма Mermaid сгенерирована в файле #{output_file}"

# Дополнительно создаем файл с информацией о модулях
modules_file = "modules_diagram.md"
modules_diagram = "# Диаграмма модулей\n\n"
modules_diagram += "```mermaid\nflowchart LR\n"

# Добавляем модули
modules.each do |mod_name, _|
  modules_diagram += "    #{mod_name.gsub('.', '_')}[\"#{mod_name}\"]\n"
end

# Добавляем связи между модулями
modules.each do |mod_name, mod_info|
  mod_info[:imports].each do |import|
    modules_diagram += "    #{mod_name.gsub('.', '_')} --> #{import.gsub('.', '_')}\n"
  end
end

modules_diagram += "```\n"

File.write(modules_file, modules_diagram)
puts "Диаграмма модулей сгенерирована в файле #{modules_file}"