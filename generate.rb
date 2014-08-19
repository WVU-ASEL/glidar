#!/usr/bin/env ruby

require 'nmatrix'
require 'nmatrix/homogeneous.rb'
require 'shellwords'

LIDAR_WIDTH  = 256  # pixels
LIDAR_HEIGHT = 256  # pixels
LIDAR_FOV    = 20   # degrees
MODEL_PATH   = 'ISS\ models\ 2011/Objects/Modules/MLM/MLM.lwo'

if ARGV.size < 6
  puts "Six arguments required: num_angles num_distances min_distance max_distance model_scale output_path"
  raise
end

# Generate a distribution of distances and rotations.
# For now, let's have both be uniform distributions.
# First argument is the number of angles to consider.
num_angles    = ARGV.shift.to_i

# Second arugment is the number of distances to consider.
num_distances = ARGV.shift.to_i

# Third argument is the minimum distance; fourth is
# the max.
min_distance  = ARGV.shift.to_f
max_distance  = ARGV.shift.to_f

# Next few arguments:
# * Model filesystem path
# * Model scale factor
# * Output directory
model_path    = MODEL_PATH #(ARGV.shift)
model_scale   = ARGV.shift.to_f
output_path   = File.path(ARGV.shift)

distances     = []

num_distances.times do
  distances << (min_distance + rand * rand * (max_distance - min_distance))
end

angles        = []

num_angles.times do
  angles    << [rand * 360.0 - 180.0,
                rand * 360.0 - 180.0,
                rand * 360.0 - 180.0]
end

count = 0
distances.each do |d|
  angles.each do |a|
    output_filename   = output_path + "/view_#{count.to_s.rjust(5, '0')}"
    info_filename     = output_path + "/info_#{count.to_s.rjust(5, '0')}.txt"
    pose_filename     = output_path + "/pose_#{count.to_s.rjust(5, '0')}.txt"

    # Generate the LIDAR image
    cmd = "build/glidar.app/Contents/MacOS/glidar #{model_path} #{model_scale} 0 0 0 #{a.join(' ')} #{d} #{LIDAR_WIDTH} #{LIDAR_HEIGHT} #{LIDAR_FOV} #{output_filename}"
    STDERR.puts "cmd is:\n#{cmd}"

    if system(cmd)
    elsif system(cmd) # Try a second time
      STDERR.puts "****** ERROR: GLIDAR HAS REPRODUCIBLE BUG."
      STDERR.puts "****** COMMAND: #{cmd}"
      STDERR.puts "Continuing for now."
    end

    # Write the distance and angles to a file.
    # On the second line give the full command string.
    f = File.open(info_filename, 'w')
    f.puts "#{d}\t" + a.join("\t")
    f.puts cmd
    f.close

    # Now write the transformation matrix to a file.
    f  = File.open(pose_filename, 'w')

    rz = NMatrix.z_rotation( a[2] * Math::PI/180.0 )
    ry = NMatrix.y_rotation( a[1] * Math::PI/180.0 )
    rx = NMatrix.z_rotation( a[0] * Math::PI/180.0 )
    t  = NMatrix.translation 0.0, 0.0, d
    transform = rx.dot(ry).dot(rz).dot(t)

    ary = transform.to_a.flatten
    f.puts ary.join(' ')

    f.close

    count += 1
  end
end



