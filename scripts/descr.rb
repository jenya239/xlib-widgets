#!/usr/bin/env ruby

# Путь к корневой директории проекта
root_dir = './'  # Путь к проекту, измените, если необходимо

# Получаем краткое описание
description = "Проект для тестирования сборки C++20 с модулями и использованием Clang. Содержит исходники, которые компилируются с помощью кастомного Ruby-скрипта.\n\n"

# Структура папок и файлов (кроме build, sort.rb и descr.rb)
description += "Структура проекта:\n"
Dir.glob("#{root_dir}**/*").sort.each do |path|
  next if path.include?('build') || path.include?('sort.rb') || path.include?('descr.rb')  # Пропускаем папку build и ненужные файлы

  if File.directory?(path)
    description += "  Директория: #{path}\n"
  else
    description += "  Файл: #{path}\n"
  end
end

# Содержимое файлов
Dir.glob("#{root_dir}**/*").sort.each do |path|
  next if path.include?('build') || path.include?('sort.rb') || path.include?('descr.rb') || !File.file?(path)  # Пропускаем папку build и ненужные файлы

  description += "\nФайл: #{path}\n"
  description += File.read(path)  # Чтение содержимого файла
end

# Запись в текстовый файл
File.open("project_description.txt", "w") do |file|
  file.puts(description)
end

puts "Описание проекта записано в project_description.txt"
