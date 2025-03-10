#!/usr/bin/env ruby
require 'fileutils'

def copy_files_recursive(source_dir, output_file, exclude_patterns = [])
  unless Dir.exist?(source_dir)
    puts "Error: Source directory '#{source_dir}' does not exist."
    return false
  end

  content = "# Directory content: #{source_dir}\n"
  content += "# Generated on: #{Time.now}\n\n"

  # Get all files recursively
  files = Dir.glob(File.join(source_dir, "**", "*")).select { |f| File.file?(f) }

  # Sort files for better readability
  files.sort!

  # Process each file
  files.each do |file|
    # Skip files matching exclude patterns
    next if exclude_patterns.any? { |pattern| file.include?(pattern) }

    # Add file separator and path
    content += "=" * 80 + "\n"
    content += "FILE: #{file}\n"
    content += "=" * 80 + "\n\n"

    # Read and add file content
    begin
      file_content = File.read(file)
      content += file_content
    rescue => e
      content += "# ERROR: Could not read file: #{e.message}\n"
    end

    content += "\n\n"
  end

  # Write the collected content to the output file
  begin
    File.write(output_file, content)
    puts "Successfully copied content of #{files.length} files to #{output_file}"
    return true
  rescue => e
    puts "Error writing to output file: #{e.message}"
    return false
  end
end

# Main execution
if ARGV.length < 2
  puts "Usage: ruby copy_files_recursive.rb SOURCE_DIRECTORY OUTPUT_FILE [EXCLUDE_PATTERN1 EXCLUDE_PATTERN2 ...]"
  puts "Example: ruby copy_files_recursive.rb src project_files.txt build .git"
  exit 1
end

source_dir = ARGV[0]
output_file = ARGV[1]
exclude_patterns = ARGV[2..-1] || []

# Add default exclude patterns
exclude_patterns << "build" unless exclude_patterns.include?("build")

copy_files_recursive(source_dir, output_file, exclude_patterns)