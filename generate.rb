#!/usr/bin/env ruby

require 'nmatrix'

LIDAR_WIDTH  = 256  # pixels
LIDAR_HEIGHT = 256  # pixels
LIDAR_FOV    = 20   # degrees

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
model_path    = ARGV.shift
model_scale   = ARGV.shift.to_f
output_path   = ARGV.shift

distances     = []

num_distances.times do
  distances << min_distance + rand * (max_distance - min_distance)
end

angles        = []

num_angles.times do
  angles    << [rand * 360.0 - 180.0,
                rand * 360.0 - 180.0,
                rand * 360.0 - 180.0]
end


class NMatrix
  class << self
    def rotate_x angle_degrees
      c = Math.cos(angle_degrees * Math::PI/180.0)
      s = Math.sin(angle_degrees * Math::PI/180.0)
      NMatrix.new(4, [1.0, 0.0, 0.0, 0.0,
                      0.0, c,   -s,  0.0,
                      0.0, s,    c,  0.0,
                      0.0, 0.0, 0.0, 1.0], dtype: :float32)
    end

    def rotate_y angle_degrees
      c = Math.cos(angle_degrees * Math::PI/180.0)
      s = Math.sin(angle_degrees * Math::PI/180.0)
      NMatrix.new(4, [ c,  0.0,  s,  0.0,
                      0.0, 1.0, 0.0, 0.0,
                      -s,  0.0,  c,  0.0,
                      0.0, 0.0, 0.0, 1.0], dtype: :float32)
    end

    def rotate_z angle_degrees
      c = Math.cos(angle_degrees * Math::PI/180.0)
      s = Math.sin(angle_degrees * Math::PI/180.0)
      NMatrix.new(4, [ c,  -s,  0.0, 0.0,
                       s,   c,  0.0, 0.0,
                      0.0, 0.0, 1.0, 0.0,
                      0.0, 0.0, 0.0, 1.0], dtype: :float32)
    end

    def translate x, y, z
      NMatrix.new(4, [1.0, 0.0, 0.0,  x,
                      0.0, 1.0, 0.0,  y,
                      0.0, 0.0, 1.0,  z,
                      0.0, 0.0, 0.0, 1.0], dtype: :float32)
    end
  end
end


count = 0
distances.each do |d|
  angles.each do |a|
    output_filename   = output_path + "/view_#{count.to_s.rjust(5, '0')}"
    info_filename     = output_path + "/info_#{count.to_s.rjust(5, '0')}.txt"
    pose_filename     = output_path + "/pose_#{count.to_s.rjust(5, '0')}.txt"

    # Generate the LIDAR image
    cmd = "./lidargl #{model_path} #{model_scale} 0 0 0 #{a.join(' ')} #{d} #{LIDAR_WIDTH} #{LIDAR_HEIGHT} #{LIDAR_FOV} #{output_filename}"
    STDERR.puts "cmd is:\n#{cmd}"
    #`#{cmd}`

    # Write the distance and angles to a file.
    # On the second line give the full command string.
    f = File.open(info_filename, 'w')
    f.puts "#{d}\t" + a.join("\t")
    f.puts cmd
    f.close

    # Now write the transformation matrix to a file.
    f  = File.open(pose_filename, 'w')

    rz = NMatrix.rotate_z a[2]
    ry = NMatrix.rotate_y a[1]
    rx = NMatrix.rotate_x a[0]
    t  = NMatrix.translate 0.0, 0.0, d
    transform = t * rx * ry * rz
    ary = t.to_a.flatten
    f.puts ary.join(' ')

    f.close
  end
end



