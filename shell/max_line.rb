root = File.expand_path(File.dirname(File.dirname(__FILE__)))
class_entries = Dir.glob("#{root}/src/**/*").reject{|f| File.directory? f }

class_entries.each do |file|
  lines = File.open(file, "r")
  File.foreach(file).with_index do |line, line_num|
    if line.size > 80
      puts "\033[31m #{file} \033[0m"
      break
    end
  end
end
