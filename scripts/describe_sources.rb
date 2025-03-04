#!/usr/bin/env ruby

require 'fileutils'

# Путь к директории с исходными файлами
src_dir = "src"
output_file = "src_description.md"

# Получаем все исходные файлы .cpp и .cppm из директории src
sources = Dir.glob("#{src_dir}/**/*.{cpp,cppm,h,hpp}")

# Формируем описание проекта
description = "# Исходные файлы проекта\n\n"

# Краткая статистика
description += "## Краткая статистика\n\n"
description += "- Всего файлов: #{sources.count}\n"
description += "- Типы файлов: #{sources.map { |f| File.extname(f) }.uniq.join(', ')}\n\n"

# Структура директорий
description += "## Структура директорий\n\n"
description += "```\n"
dirs = {}
sources.each do |file|
  dir = File.dirname(file)
  dirs[dir] ||= []
  dirs[dir] << File.basename(file)
end

dirs.keys.sort.each do |dir|
  description += "#{dir}/\n"
  dirs[dir].sort.each do |file|
    description += "  └── #{file}\n"
  end
end
description += "```\n\n"

# Полное содержимое файлов
description += "## Содержимое файлов\n\n"

sources.sort.each do |file|
  description += "### Файл: `#{file}`\n\n"

  # Определяем язык для подсветки синтаксиса в Markdown
  ext = File.extname(file)
  lang = case ext
         when '.cpp', '.cppm' then 'cpp'
         when '.h', '.hpp' then 'cpp'
         else 'text'
         end

  # Добавляем полное содержимое файла
  content = File.read(file)
  description += "```#{lang}\n#{content}\n```\n\n"

  # Добавляем разделитель между файлами для лучшей читаемости
  description += "---\n\n"
end

# Запись в файл
File.write(output_file, description)
puts "Полное описание исходных файлов записано в #{output_file}"