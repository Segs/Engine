using System;
using System.Runtime.InteropServices;

namespace Godot
{
    /// <summary>
    /// A unit quaternion used for representing 3D rotations.
    /// Quaternions need to be normalized to be used for rotation.
    ///
    /// It is similar to Basis, which implements matrix representation of
    /// rotations, and can be parametrized using both an axis-angle pair
    /// or Euler angles. Basis stores rotation, scale, and shearing,
    /// while Quat only stores rotation.
    ///
    /// Due to its compactness and the way it is stored in memory, certain
    /// operations (obtaining axis-angle and performing SLERP, in particular)
    /// are more efficient and robust against floating-point errors.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Quat : IEquatable<Quat>
    {
        /// <summary>
        /// X component of the quaternion (imaginary `i` axis part).
        /// Quaternion components should usually not be manipulated directly.
        /// </summary>
        public float x;
        /// <summary>
        /// Y component of the quaternion (imaginary `j` axis part).
        /// Quaternion components should usually not be manipulated directly.
        /// </summary>
        public float y;
        /// <summary>
        /// Z component of the quaternion (imaginary `k` axis part).
        /// Quaternion components should usually not be manipulated directly.
        /// </summary>
        public float z;
        /// <summary>
        /// W component of the quaternion (real part).
        /// Quaternion components should usually not be manipulated directly.
        /// </summary>
        public float w;

        /// <summary>
        /// Access quaternion components using their index.
        /// </summary>
        /// <value>`[0]` is equivalent to `.x`, `[1]` is equivalent to `.y`, `[2]` is equivalent to `.z`, `[3]` is equivalent to `.w`.</value>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0:
                        return x;
                    case 1:
                        return y;
                    case 2:
                        return z;
                    case 3:
                        return w;
                    default:
                        throw new IndexOutOfRangeException();
                }
            }
            set
            {
                switch (index)
                {
                    case 0:
                        x = value;
                        break;
                    case 1:
                        y = value;
                        break;
                    case 2:
                        z = value;
                        break;
                    case 3:
                        w = value;
                        break;
                    default:
                        throw new IndexOutOfRangeException();
                }
            }
        }

        /// <summary>
        /// Returns the length (magnitude) of the quaternion.
        /// </summary>
        /// <value>Equivalent to `Mathf.Sqrt(LengthSquared)`.</value>
        public float Length
        {
            get { return Mathf.Sqrt(LengthSquared); }
        }

        /// <summary>
        /// Returns the squared length (squared magnitude) of the quaternion.
        /// This method runs faster than <see cref="Length"/>, so prefer it if
        /// you need to compare quaternions or need the squared length for some formula.
        /// </summary>
        /// <value>Equivalent to `Dot(this)`.</value>
        public float LengthSquared
        {
            get { return Dot(this); }
        }

        /// <summary>
        /// Performs a cubic spherical interpolation between quaternions `preA`,
        /// this vector, `b`, and `postB`, by the given amount `t`.
        /// </summary>
        /// <param name="b">The destination quaternion.</param>
        /// <param name="preA">A quaternion before this quaternion.</param>
        /// <param name="postB">A quaternion after `b`.</param>
        /// <param name="t">A value on the range of 0.0 to 1.0, representing the amount of interpolation.</param>
        /// <returns>The interpolated quaternion.</returns>
        public Quat CubicSlerp(Quat b, Quat preA, Quat postB, float t)
        {
            float t2 = (1.0f - t) * t * 2f;
            Quat sp = Slerp(b, t);
            Quat sq = preA.Slerpni(postB, t);
            return sp.Slerpni(sq, t2);
        }

        /// <summary>
        /// Returns the dot product of two quaternions.
        /// </summary>
        /// <param name="b">The other quaternion.</param>
        /// <returns>The dot product.</returns>
        public float Dot(Quat b)
        {
            return x * b.x + y * b.y + z * b.z + w * b.w;
        }

        /// <summary>
        /// Returns Euler angles (in the YXZ convention: when decomposing,
        /// first Z, then X, and Y last) corresponding to the rotation
        /// represented by the unit quaternion. Returned vector contains
        /// the rotation angles in the format (X angle, Y angle, Z angle).
        /// </summary>
        /// <returns>The Euler angle representation of this quaternion.</returns>
        public Vector3 GetEuler()
        {
#if DEBUG
            if (!IsNormalized())
            {
                throw new InvalidOperationException("Quat is not normalized");
            }
#endif
            var basis = new Basis(this);
            return basis.GetEuler();
        }

        /// <summary>
        /// Returns the inverse of the quaternion.
        /// </summary>
        /// <returns>The inverse quaternion.</returns>
        public Quat Inverse()
        {
#if DEBUG
            if (!IsNormalized())
            {
                throw new InvalidOperationException("Quat is not normalized");
            }
#endif
            return new Quat(-x, -y, -z, w);
        }

        /// <summary>
        /// Returns whether the quaternion is normalized or not.
        /// </summary>
        /// <returns>A bool for whether the quaternion is normalized or not.</returns>
        public bool IsNormalized()
        {
            return Mathf.Abs(LengthSquared - 1) <= Mathf.Epsilon;
        }

        /// <summary>
        /// Returns a copy of the quaternion, normalized to unit length.
        /// </summary>
        /// <returns>The normalized quaternion.</returns>
        public Quat Normalized()
        {
            return this / Length;
        }

        [Obsolete("Set is deprecated. Use the Quat(float,float,float,float) constructor instead.", error: true)]
        public void Set(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        [Obsolete("Set is deprecated. Use the Quat(" + nameof(Quat) + ") constructor instead.", error: true)]
        public void Set(Quat q)
        {
            this = q;
        }

        [Obsolete("SetAxisAngle is deprecated. Use the Quat(" + nameof(Vector3) + ", float) constructor instead.", error: true)]
        public void SetAxisAngle(Vector3 axis, float angle)
        {
            this = new Quat(axis, angle);
        }

        [Obsolete("SetEuler is deprecated. Use the Quat(" + nameof(Vector3) + ") constructor instead.", error: true)]
        public void SetEuler(Vector3 eulerYXZ)
        {
            this = new Quat(eulerYXZ);
        }

        /// <summary>
        /// Returns the result of the spherical linear interpolation between
        /// this quaternion and `to` by amount `weight`.
        ///
        /// Note: Both quaternions must be normalized.
        /// </summary>
        /// <param name="to">The destination quaternion for interpolation. Must be normalized.</param>
        /// <param name="weight">A value on the range of 0.0 to 1.0, representing the amount of interpolation.</param>
        /// <returns>The resulting quaternion of the interpolation.</returns>
        public Quat Slerp(Quat to, float weight)
        {
#if DEBUG
            if (!IsNormalized())
            {
                throw new InvalidOperationException("Quat is not normalized");
            }
            if (!to.IsNormalized())
            {
                throw new ArgumentException("Argument is not normalized", nameof(to));
            }
#endif

            // Calculate cosine.
            float cosom = x * to.x + y * to.y + z * to.z + w * to.w;

            var to1 = new Quat();

            // Adjust signs if necessary.
            if (cosom < 0.0)
            {
                cosom = -cosom;
                to1.x = -to.x;
                to1.y = -to.y;
                to1.z = -to.z;
                to1.w = -to.w;
            }
            else
            {
                to1.x = to.x;
                to1.y = to.y;
                to1.z = to.z;
                to1.w = to.w;
            }

            float sinom, scale0, scale1;

            // Calculate coefficients.
            if (1.0 - cosom > Mathf.Epsilon)
            {
                // Standard case (Slerp).
                float omega = Mathf.Acos(cosom);
                sinom = Mathf.Sin(omega);
                scale0 = Mathf.Sin((1.0f - weight) * omega) / sinom;
                scale1 = Mathf.Sin(weight * omega) / sinom;
            }
            else
            {
                // Quaternions are very close so we can do a linear interpolation.
                scale0 = 1.0f - weight;
                scale1 = weight;
            }

            // Calculate final values.
            return new Quat
            (
                scale0 * x + scale1 * to1.x,
                scale0 * y + scale1 * to1.y,
                scale0 * z + scale1 * to1.z,
                scale0 * w + scale1 * to1.w
            );
        }

        /// <summary>
        /// Returns the result of the spherical linear interpolation between
        /// this quaternion and `to` by amount `weight`, but without
        /// checking if the rotation path is not bigger than 90 degrees.
        /// </summary>
        /// <param name="to">The destination quaternion for interpolation. Must be normalized.</param>
        /// <param name="weight">A value on the range of 0.0 to 1.0, representing the amount of interpolation.</param>
        /// <returns>The resulting quaternion of the interpolation.</returns>
        public Quat Slerpni(Quat to, float weight)
        {
            float dot = Dot(to);

            if (Mathf.Abs(dot) > 0.9999f)
            {
                return this;
            }

            float theta = Mathf.Acos(dot);
            float sinT = 1.0f / Mathf.Sin(theta);
            float newFactor = Mathf.Sin(weight * theta) * sinT;
            float invFactor = Mathf.Sin((1.0f - weight) * theta) * sinT;

            return new Quat
            (
                invFactor * x + newFactor * to.x,
                invFactor * y + newFactor * to.y,
                invFactor * z + newFactor * to.z,
                invFactor * w + newFactor * to.w
            );
        }

        /// <summary>
        /// Returns a vector transformed (multiplied) by this quaternion.
        /// </summary>
        /// <param name="v">A vector to transform.</param>
        /// <returns>The transformed vector.</returns>
        public Vector3 Xform(Vector3 v)
        {
#if DEBUG
            if (!IsNormalized())
            {
                throw new InvalidOperationException("Quat is not normalized");
            }
#endif
            var u = new Vector3(x, y, z);
            Vector3 uv = u.Cross(v);
            return v + ((uv * w) + u.Cross(uv)) * 2;
        }

        // Constants
        private static readonly Quat _identity = new Quat(0, 0, 0, 1);

        /// <summary>
        /// The identity quaternion, representing no rotation.
        /// Equivalent to an identity <see cref="Basis"/> matrix. If a vector is transformed by
        /// an identity quaternion, it will not change.
        /// </summary>
        /// <value>Equivalent to `new Quat(0, 0, 0, 1)`.</value>
        public static Quat Identity { get { return _identity; } }

        /// <summary>
        /// Constructs a quaternion defined by the given values.
        /// </summary>
        /// <param name="x">X component of the quaternion (imaginary `i` axis part).</param>
        /// <param name="y">Y component of the quaternion (imaginary `j` axis part).</param>
        /// <param name="z">Z component of the quaternion (imaginary `k` axis part).</param>
        /// <param name="w">W component of the quaternion (real part).</param>
        public Quat(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        /// <summary>
        /// Constructs a quaternion from the given quaternion.
        /// </summary>
        /// <param name="q">The existing quaternion.</param>

        public Quat(Quat q)
        {
            this = q;
        }

        /// <summary>
        /// Constructs a quaternion from the given <see cref="Basis"/>.
        /// </summary>
        /// <param name="basis">The basis to construct from.</param>
        public Quat(Basis basis)
        {
            this = basis.Quat();
        }

        /// <summary>
        /// Constructs a quaternion that will perform a rotation specified by
        /// Euler angles (in the YXZ convention: when decomposing,
        /// first Z, then X, and Y last),
        /// given in the vector format as (X angle, Y angle, Z angle).
        /// </summary>
        /// <param name="eulerYXZ"></param>
        public Quat(Vector3 eulerYXZ)
        {
            float half_a1 = eulerYXZ.y * 0.5f;
            float half_a2 = eulerYXZ.x * 0.5f;
            float half_a3 = eulerYXZ.z * 0.5f;

            // R = Y(a1).X(a2).Z(a3) convention for Euler angles.
            // Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-6)
            // a3 is the angle of the first rotation, following the notation in this reference.

            float cos_a1 = Mathf.Cos(half_a1);
            float sin_a1 = Mathf.Sin(half_a1);
            float cos_a2 = Mathf.Cos(half_a2);
            float sin_a2 = Mathf.Sin(half_a2);
            float cos_a3 = Mathf.Cos(half_a3);
            float sin_a3 = Mathf.Sin(half_a3);

            x = sin_a1 * cos_a2 * sin_a3 + cos_a1 * sin_a2 * cos_a3;
            y = sin_a1 * cos_a2 * cos_a3 - cos_a1 * sin_a2 * sin_a3;
            z = cos_a1 * cos_a2 * sin_a3 - sin_a1 * sin_a2 * cos_a3;
            w = sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3;
        }

        /// <summary>
        /// Constructs a quaternion that will rotate around the given axis
        /// by the specified angle. The axis must be a normalized vector.
        /// </summary>
        /// <param name="axis">The axis to rotate around. Must be normalized.</param>
        /// <param name="angle">The angle to rotate, in radians.</param>
        public Quat(Vector3 axis, float angle)
        {
#if DEBUG
            if (!axis.IsNormalized())
            {
                throw new ArgumentException("Argument is not normalized", nameof(axis));
            }
#endif

            float d = axis.Length();

            if (d == 0f)
            {
                x = 0f;
                y = 0f;
                z = 0f;
                w = 0f;
            }
            else
            {
                float sinAngle = Mathf.Sin(angle * 0.5f);
                float cosAngle = Mathf.Cos(angle * 0.5f);
                float s = sinAngle / d;

                x = axis.x * s;
                y = axis.y * s;
                z = axis.z * s;
                w = cosAngle;
            }
        }

        public static Quat operator *(Quat left, Quat right)
        {
            return new Quat
            (
                left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
                left.w * right.y + left.y * right.w + left.z * right.x - left.x * right.z,
                left.w * right.z + left.z * right.w + left.x * right.y - left.y * right.x,
                left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z
            );
        }

        public static Quat operator +(Quat left, Quat right)
        {
            return new Quat(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
        }

        public static Quat operator -(Quat left, Quat right)
        {
            return new Quat(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
        }

        public static Quat operator -(Quat left)
        {
            return new Quat(-left.x, -left.y, -left.z, -left.w);
        }

        public static Quat operator *(Quat left, Vector3 right)
        {
            return new Quat
            (
                left.w * right.x + left.y * right.z - left.z * right.y,
                left.w * right.y + left.z * right.x - left.x * right.z,
                left.w * right.z + left.x * right.y - left.y * right.x,
                -left.x * right.x - left.y * right.y - left.z * right.z
            );
        }

        public static Quat operator *(Vector3 left, Quat right)
        {
            return new Quat
            (
                right.w * left.x + right.y * left.z - right.z * left.y,
                right.w * left.y + right.z * left.x - right.x * left.z,
                right.w * left.z + right.x * left.y - right.y * left.x,
                -right.x * left.x - right.y * left.y - right.z * left.z
            );
        }

        public static Quat operator *(Quat left, float right)
        {
            return new Quat(left.x * right, left.y * right, left.z * right, left.w * right);
        }

        public static Quat operator *(float left, Quat right)
        {
            return new Quat(right.x * left, right.y * left, right.z * left, right.w * left);
        }

        public static Quat operator /(Quat left, float right)
        {
            return left * (1.0f / right);
        }

        public static bool operator ==(Quat left, Quat right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(Quat left, Quat right)
        {
            return !left.Equals(right);
        }

        public override bool Equals(object obj)
        {
            if (obj is Quat)
            {
                return Equals((Quat)obj);
            }

            return false;
        }

        public bool Equals(Quat other)
        {
            return x == other.x && y == other.y && z == other.z && w == other.w;
        }

        /// <summary>
        /// Returns true if this quaternion and `other` are approximately equal, by running
        /// <see cref="Mathf.IsEqualApprox(float, float)"/> on each component.
        /// </summary>
        /// <param name="other">The other quaternion to compare.</param>
        /// <returns>Whether or not the quaternions are approximately equal.</returns>
        public bool IsEqualApprox(Quat other)
        {
            return Mathf.IsEqualApprox(x, other.x) && Mathf.IsEqualApprox(y, other.y) && Mathf.IsEqualApprox(z, other.z) && Mathf.IsEqualApprox(w, other.w);
        }

        public override int GetHashCode()
        {
            return y.GetHashCode() ^ x.GetHashCode() ^ z.GetHashCode() ^ w.GetHashCode();
        }

        public override string ToString()
        {
            return String.Format("({0}, {1}, {2}, {3})", x.ToString(), y.ToString(), z.ToString(), w.ToString());
        }

        public string ToString(string format)
        {
            return String.Format("({0}, {1}, {2}, {3})", x.ToString(format), y.ToString(format), z.ToString(format), w.ToString(format));
        }
    }
}
