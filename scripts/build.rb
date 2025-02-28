require 'set'
require 'fileutils'

# Директория с исходными файлами
src_dir = "src"
output_dir = "build"  # Папка для выходных файлов
temp_dir = "#{output_dir}/modules"  # Папка для промежуточных файлов (PCM)
main_file = 'main.cpp'
output_executable_name = 'app'

# Создаем необходимые директории
FileUtils.mkdir_p(temp_dir)

# Получаем все исходные файлы .cpp и .cppm из директории src
sources = Dir.glob("#{src_dir}/**/*.{cpp,cppm}")

modules = {}  # { module_name => { file: "file.cppm", imports: ["OtherModule", ...] } }

# Собираем информацию о модулях и их зависимостях
sources.each do |file|
  content = File.read(file)
  
  # Находим модуль (если есть)
  module_name = content[/^\s*export\s+module\s+([\w\.]+)/, 1]
  
  # Находим импорты
  imports = content.scan(/^\s*import\s+([\w\.]+)/).flatten
  
  modules[module_name] = { file: file, imports: imports } if module_name
end

# Топологическая сортировка
sorted = []
visited = Set.new

dfs = ->(mod) {
  return if visited.include?(mod)
  visited << mod
  modules[mod][:imports].each { |dep| dfs.call(dep) if modules[dep] }
  sorted << mod
}

modules.keys.each { |mod| dfs.call(mod) }

# Формируем команды для сборки
build_commands = []

compiled_modules = {}

sorted.each do |mod|
  file = modules[mod][:file]
  output_file = File.join(temp_dir, "#{mod}.pcm")

  # Собираем список уже скомпилированных зависимостей
  deps_flags = modules[mod][:imports].map { |dep| "-fmodule-file=#{dep}=#{compiled_modules[dep]}" if compiled_modules[dep] }.compact.join(' ')

  # Команда для предварительной компиляции модуля с зависимостями
  build_commands << "clang++ -std=c++20 #{file} --precompile #{deps_flags} -o #{output_file}"

  # Запоминаем путь к скомпилированному .pcm
  compiled_modules[mod] = output_file
end

# Команда для сборки основного исполнимого файла
main_file = "#{src_dir}/#{main_file}"  # Основной исходник должен быть в директории src
output_executable = File.join(output_dir, output_executable_name)

# Здесь `module_map` — хеш, где ключ — имя модуля, значение — путь к .pcm
module_map = sorted.map { |mod_name| [mod_name, File.join(temp_dir, "#{mod_name}.pcm")] }.to_h

# Формируем аргументы
module_files_flags = module_map.map { |name, path| "-fmodule-file=#{name}=#{path}" }.join(' ')
module_files_list  = module_map.values.join(' ')

# Итоговая команда
build_commands << "clang++ -std=c++20 #{main_file} #{module_files_flags} #{module_files_list} -lX11 -o #{output_executable}"


# Выводим команды для выполнения
puts "Сгенерированные команды для сборки:"
build_commands.each { |cmd| puts cmd }

# Запуск сборки с захватом вывода ошибок
puts "Запуск сборки..."
error_log = ''  # Для хранения вывода ошибок

build_commands.each do |cmd|
  # Выполняем команду и захватываем вывод ошибок
  result = `#{cmd} 2>&1`
  
  if $?.success?
    puts "Команда выполнена успешно: #{cmd}"
  else
    error_log += "Ошибка при выполнении команды: #{cmd}\n"
    error_log += result + "\n"
  end
end

# Если были ошибки
if error_log.empty?
  puts "Сборка завершена успешно!"
else
  puts "Сборка завершена с ошибками:"
  puts error_log
end
