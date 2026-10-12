#pragma once

#include <cmath>

#undef min
#undef max

namespace Mesh
{
	struct Vector2
	{
		float x{ 0.f }, y{ 0.f };
		Vector2() = default;
		Vector2(float x, float y) : x(x), y(y) {}
	};

	struct Vector3
	{
		float x{ 0.f }, y{ 0.f }, z{ 0.f };
		Vector3() = default;
		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

		Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
		Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
		Vector3 operator*(float s)          const { return { x * s, y * s, z * s }; }
		Vector3 operator/(float s)          const { return { x / s, y / s, z / s }; }
		Vector3 operator-() const { return { -x, -y, -z }; }

		float   Length()      const { return std::sqrt(x * x + y * y + z * z); }
		Vector3 Normalized()  const
		{
			const float len = Length();
			return len > 0.f ? *this / len : Vector3();
		}
		float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
		Vector3 Cross(const Vector3& o) const
		{
			return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
		}
	};

	struct Matrix4x4
	{
		float m[4][4]{};

		Matrix4x4()
		{
			m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
		}

		Matrix4x4(float m00, float m01, float m02, float m03,
		          float m10, float m11, float m12, float m13,
		          float m20, float m21, float m22, float m23,
		          float m30, float m31, float m32, float m33)
		{
			m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
			m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
			m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
			m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
		}
	};
}
