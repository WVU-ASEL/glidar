require "nmatrix"

class Quaternion
  class << self
    def vector_cross(x, y)
      NMatrix[[x[1,0]*y[2,0] - x[2,0]*y[1,0]],
              [x[2,0]*y[0,0] - x[0,0]*y[2,0]],
              [x[0,0]*y[1,0] - x[1,0]*y[0,0]]]
    end

    def identity
      Quaternion.new(1.0, 0.0, 0.0, 0.0)
    end


    # Subgroup algorithm (Shoemake 1992: "Uniform random rotations" in Graphics Gems III, pp. 124-132)
    # Produces an uniform distribution of unit quaternions.
    def rand
      u1 = Random.rand(1.0)
      u2 = Random.rand(2.0*Math::PI)
      u3 = Random.rand(2.0*Math::PI)
      Quaternion.new(Math.sqrt(u1)*Math.cos(u3),
        Math.sqrt(1.0-u1)*Math.sin(u2),
        Math.sqrt(1.0-u1)*Math.cos(u2),
        Math.sqrt(u1)*Math.sin(u3))
    end

    # Create a quaternion from an axis and an angle
    def from_axis_angle axis, angle
      if axis.is_a?(Numeric) # Switch them if they're in the wrong order
        tmp = axis
        axis = angle
        angle = tmp
      end
      axis = NMatrix[axis].transpose unless axis.is_a?(NMatrix)
      q_axis = axis * Math.sin(angle/2.0)
      qw = Math.cos(angle/2.0)
      Quaternion.new(qw, q_axis[0], q_axis[1], q_axis[2])
    end
  end
  
  def initialize *args
    @w,@x,@y,@z = args.map { |a| a.to_f }
  end
  attr_accessor :w, :x, :y, :z

  def vec
    NMatrix[[@x], [@y], [@z]]
  end
  
  def cross q
    p = self
    new_w   = p.w * q.w - (p.x * q.x + p.y * q.y + p.z * q.z)
    new_vec = p.vec * q.w + q.vec * p.w - Quaternion.vector_cross(p.vec, q.vec)    
    Quaternion.new(new_w, new_vec[0,0], new_vec[1,0], new_vec[2,0])
  end

  def normalize!
    magnitude = Math.sqrt(x*x + y*y + z*z)
    @w /= magnitude
    @x /= magnitude
    @y /= magnitude
    @z /= magnitude
    self
  end

  def normalize
    magnitude = Math.sqrt(x*x + y*y + z*z)
    Quaternion.new(@w / magnitude, @x / magnitude, @y / magnitude, @z / magnitude)
  end

  def conjugate
    Quaternion.new(@w, -@x, -@y, -@z)
  end

  def conjugate!
    @x = -@x
    @y = -@y
    @z = -@z
    self
  end

  def to_axis
    [@x / @w, @y / @w, @z / @w]
  end

  def vec
    NMatrix[[@x],[@y],[@z]]
  end

  def vec_cross
    NMatrix[[0.0, -@z,  @y],
            [@z,  0.0, -@x],
            [-@y, @x,  0.0]]
  end

  def to_rotation_matrix
    v = self.vec
    NMatrix.eye(3) * (@w*@w - (v.transpose.dot(v))[0,0]) - vec_cross*2.0*@w + v.dot(v.transpose)*2.0
  end

  def to_a
    [@w, @x, @y, @z]
  end

  def * q
    p = self
    w = p.w * q.w - p.vec.transpose.dot(q.vec)[0,0]
    v = q.vec * p.w + p.vec * q.w - p.vec_cross.dot(q.vec)
    Quaternion.new(w, v[0], v[1], v[2])
  end

  def to_euler_angles # in degrees
    [Math.atan2(2.0 * (@w*@x + @y*@z), 1.0 - 2.0*(@x*@x + @y*@y)) * 180.0 / Math::PI,
     Math.asin(2.0 * (@w*@y - @z*@x)) * 180.0 / Math::PI,
     Math.atan2(2.0 * (@w*@z + @x*@y), 1.0 - 20*(@y*@y + @z*@z)) * 180.0 / Math::PI]
  end
end
