require 'set'

modules = {} # { module_name => { file: "file.cppm", imports: ["OtherModule", ...] } }
sources = Dir.glob("**/*.{cpp,cppm}")

sources.each do |file|
  content = File.read(file)
  
  # Находим модуль (если есть)
  module_name = content[/^\s*export\s+module\s+(\w+)/, 1]
  
  # Находим импорты
  imports = content.scan(/^\s*import\s+(\w+)/).flatten
  
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

# Вывод порядка сборки
sorted.each { |mod| puts "#{modules[mod][:file]} -> #{mod}" }
