require 'set'
require 'fileutils'
require 'digest'
require 'json'

# Добавляем флаги для FreeType и Xft
freetype_flags = "`pkg-config --cflags freetype2`".strip
xft_flags = "`pkg-config --cflags xft`".strip
x11_flags = "`pkg-config --cflags x11`".strip
compile_flags = "#{freetype_flags} #{xft_flags} #{x11_flags}"

# Директория с исходными файлами
src_dir = "src"
output_dir = "build"  # Папка для выходных файлов
temp_dir = "#{output_dir}/modules"  # Папка для промежуточных файлов (PCM)
main_file = 'main.cpp'
output_executable_name = 'app'
cache_file = "#{output_dir}/build_cache.json"

# Создаем необходимые директории
FileUtils.mkdir_p(temp_dir)

# Загружаем кэш предыдущей сборки, если он существует
cache = {}
if File.exist?(cache_file)
  begin
    cache = JSON.parse(File.read(cache_file))
  rescue JSON::ParserError
    puts "Предупреждение: Кэш поврежден, будет создан новый"
    cache = {}
  end
end

# Получаем все исходные файлы .cpp и .cppm из директории src
sources = Dir.glob("#{src_dir}/**/*.{cpp,cppm}")

modules = {}  # { module_name => { file: "file.cppm", imports: ["OtherModule", ...], checksum: "md5hash" } }
file_checksums = {}  # { file_path => checksum }

# Вычисляем контрольные суммы для всех файлов
sources.each do |file|
  content = File.read(file)
  checksum = Digest::MD5.hexdigest(content)
  file_checksums[file] = checksum

  # Находим модуль (если есть)
  module_name = content[/^\s*export\s+module\s+([\w\.]+)/, 1]

  # Находим импорты
  imports = content.scan(/^\s*import\s+([\w\.]+)/).flatten

  modules[module_name] = { file: file, imports: imports, checksum: checksum } if module_name
end

# Проверяем main файл
main_path = "#{src_dir}/#{main_file}"
if File.exist?(main_path)
  file_checksums[main_path] = Digest::MD5.hexdigest(File.read(main_path))
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

# Определяем, какие модули нужно пересобрать
modules_to_rebuild = Set.new

# Проверяем, изменились ли сами модули
modules.each do |mod_name, mod_info|
  old_checksum = cache.dig(mod_name, "checksum")
  if old_checksum != mod_info[:checksum]
    modules_to_rebuild.add(mod_name)
  end
end

# Проверяем зависимости - если зависимость изменилась, нужно пересобрать модуль
sorted.each do |mod|
  next if modules_to_rebuild.include?(mod)

  modules[mod][:imports].each do |dep|
    if modules_to_rebuild.include?(dep)
      modules_to_rebuild.add(mod)
      break
    end
  end
end

# Формируем команды для сборки
build_commands = []
compiled_modules = {}

# Загружаем информацию о ранее скомпилированных модулях, которые не требуют пересборки
modules.each do |mod_name, mod_info|
  unless modules_to_rebuild.include?(mod_name)
    pcm_path = File.join(temp_dir, "#{mod_name}.pcm")
    if File.exist?(pcm_path)
      compiled_modules[mod_name] = pcm_path
    else
      # Если файл .pcm не существует, нужно пересобрать модуль
      modules_to_rebuild.add(mod_name)
    end
  end
end

# Собираем модули, которые требуют пересборки
sorted.each do |mod|
  next unless modules_to_rebuild.include?(mod)

  file = modules[mod][:file]
  output_file = File.join(temp_dir, "#{mod}.pcm")

  # Собираем список уже скомпилированных зависимостей
  deps_flags = modules[mod][:imports].map { |dep| "-fmodule-file=#{dep}=#{compiled_modules[dep]}" if compiled_modules[dep] }.compact.join(' ')

  # Команда для предварительной компиляции модуля с зависимостями
  build_commands << "clang++ -std=c++20 #{file} --precompile -fprebuilt-module-path=#{temp_dir} #{deps_flags} #{compile_flags} -o #{output_file}" # -lX11

  # Запоминаем путь к скомпилированному .pcm
  compiled_modules[mod] = output_file
end

# Проверяем, нужно ли пересобирать основной исполняемый файл
main_path = "#{src_dir}/#{main_file}"
rebuild_main = false

if !File.exist?(File.join(output_dir, output_executable_name))
  rebuild_main = true
elsif cache["main_checksum"] != file_checksums[main_path]
  rebuild_main = true
else
  # Проверяем, изменились ли какие-либо модули
  rebuild_main = !modules_to_rebuild.empty?
end

# Команда для сборки основного исполнимого файла
if rebuild_main
  output_executable = File.join(output_dir, output_executable_name)

  # Здесь `module_map` — хеш, где ключ — имя модуля, значение — путь к .pcm
  module_map = sorted.map { |mod_name| [mod_name, compiled_modules[mod_name]] }.to_h

  # Формируем аргументы
  module_files_flags = module_map.map { |name, path| "-fmodule-file=#{name}=#{path}" }.join(' ')
  module_files_list  = module_map.values.join(' ')

  # Итоговая команда
  build_commands << "clang++ -std=c++20 #{main_path} #{module_files_flags} #{module_files_list} -lX11 -lXft -lfreetype -o #{output_executable}"
end

# Обновляем кэш
new_cache = {}
modules.each do |mod_name, mod_info|
  new_cache[mod_name] = {
    "file" => mod_info[:file],
    "checksum" => mod_info[:checksum],
    "imports" => mod_info[:imports]
  }
end
new_cache["main_checksum"] = file_checksums[main_path] if File.exist?(main_path)

# Выводим команды для выполнения
if build_commands.empty?
  puts "Все файлы актуальны, сборка не требуется."
else
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
    # Сохраняем обновленный кэш только при успешной сборке
    File.write(cache_file, JSON.pretty_generate(new_cache))
  else
    puts "Сборка завершена с ошибками:"
    puts error_log
  end
end