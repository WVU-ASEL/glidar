
36.times do |t|
  next if t == 0
  `./lidargl n20r4_d5.obj 100.0 0 0 0 0 #{t*10} 0 400.0 256 256 rock#{t}.pcd`
end
