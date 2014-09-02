#!/usr/bin/env ruby

require 'nmatrix'
require 'nmatrix/homogeneous.rb'
require 'shellwords'

LIDAR_WIDTH  = 256  # pixels
LIDAR_HEIGHT = 256  # pixels
LIDAR_FOV    = 20   # degrees
MODEL_PATH   = 'ISS\ models\ 2011/Objects/Modules/MLM/MLM.lwo'
BINARY       = "build/glidar" #"build/glidar.app/Contents/MacOS/glidar"

if ARGV.size < 5
  puts "Five arguments required: num_tests min_distance max_distance model_scale output_path"
  raise
end

# Generate a distribution of distances and rotations.
# First argument is the number of angles to consider.
num_tests     = ARGV.shift.to_i

# The minimum and maximum distances.
min_distance  = ARGV.shift.to_f
max_distance  = ARGV.shift.to_f

# Next few arguments:
# * Model filesystem path
# * Model scale factor
# * Output directory
model_path    = MODEL_PATH #(ARGV.shift)
model_scale   = ARGV.shift.to_f
output_path   = File.path(ARGV.shift)

positions     = []
num_tests.times do
  positions << [rand * 360.0 - 180.0,
                rand * 360.0 - 180.0,
                rand * 360.0 - 180.0,
                min_distance + rand * rand * (max_distance - min_distance)
               ]
end

log = File.new("generate.log", "w")

positions.each.with_index do |p,count|
  output_filename   = output_path + "/view_#{count.to_s.rjust(5, '0')}"
  info_filename     = output_path + "/info_#{count.to_s.rjust(5, '0')}.txt"
  pose_filename     = output_path + "/pose_#{count.to_s.rjust(5, '0')}.txt"

  # Generate the LIDAR image
  cmd = "#{BINARY} #{model_path} #{model_scale} 0 0 0 #{p.join(' ')} #{LIDAR_WIDTH} #{LIDAR_HEIGHT} #{LIDAR_FOV} #{output_filename}"
  STDERR.puts "cmd is:\n#{cmd}"

  if !system(cmd)
    log.puts "Failure (#{count}): #{cmd}"
    if !system(cmd) # Try a second time
      log.puts "Failure #2 (#{count}): #{cmd}"
      STDERR.puts "****** ERROR: GLIDAR HAS REPRODUCIBLE BUG."
      STDERR.puts "****** COMMAND: #{cmd}"
      STDERR.puts "Continuing for now."
    end
  end

  # Write the distance and angles to a file.
  # On the second line give the full command string.
  f = File.open(info_filename, 'w')
  f.puts p.join("\t")
  f.puts cmd
  f.close

  # Now write the transformation matrix to a file.
  #f  = File.open(pose_filename, 'w')

  #rz = NMatrix.z_rotation( a[2] * Math::PI/180.0 )
  #ry = NMatrix.y_rotation( a[1] * Math::PI/180.0 )
  #rx = NMatrix.z_rotation( a[0] * Math::PI/180.0 )
  #t  = NMatrix.translation 0.0, 0.0, d
  #transform = rx.dot(ry).dot(rz).dot(t)

  #ary = transform.to_a.flatten
  #f.puts ary.join(' ')

  #f.close
end

log.close



