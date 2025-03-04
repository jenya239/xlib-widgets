#!/usr/bin/env ruby

require 'fileutils'

# Функция для сбора содержимого файлов
def collect_files(file_list, output_file)
  content = ""

  file_list.each do |file|
    if File.exist?(file)
      content += "=" * 80 + "\n"
      content += "FILE: #{file}\n"
      content += "=" * 80 + "\n\n"
      content += File.read(file)
      content += "\n\n"
    else
      puts "Предупреждение: файл #{file} не найден"
    end
  end

  # Записываем собранное содержимое в выходной файл
  File.write(output_file, content)
  puts "Содержимое файлов собрано в #{output_file}"
end

# Проверяем аргументы командной строки
if ARGV.length < 2
  puts "Использование: ruby collect_files.rb output_file.txt file1 file2 ..."
  exit 1
end

output_file = ARGV[0]
file_list = ARGV[1..-1]

collect_files(file_list, output_file)