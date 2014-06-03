
36.times do |t|
  next if t == 0
  output = "bunny" + t.to_s.rjust(2, '0')
  `./lidargl bun_zipper.ply 1000.0 0 0 0 0 #{t*10} 0 655.0 256 256 20 #{output}`
end
