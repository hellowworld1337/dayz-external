#pragma once

namespace Math
{
	struct Vector3D
	{
	public:
		Vector3D()
			: X(0.f)
			, Y(0.f)
			, Z(0.f)
		{ }

		Vector3D(float InX, float InY, float InZ)
			: X(InX)
			, Y(InY)
			, Z(InZ)
		{ }

		Vector3D(const Vector3D& Vector)
			: X(Vector.X)
			, Y(Vector.Y)
			, Z(Vector.Z)
		{ }

		Vector3D(const float* clr)
		{
			X = clr[0];
			Y = clr[1];
			Z = clr[2];
		}

		void Set(float InX, float InY, float InZ)
		{
			X = InX;
			Y = InY;
			Z = InZ;
		}


		float Dot(const Vector3D& Vector) const
		{
			return (X * Vector.X + Y * Vector.Y + Z * Vector.Z);
		}

		float Size() const
		{
			const auto Squared = SizeSquared();
			return std::sqrt(Squared);
		}

		float SizeSquared() const
		{
			return (X * X + Y * Y + Z * Z);
		}

		float Size2D() const
		{
			const auto Squared2D = SizeSquared2D();
			return std::sqrt(Squared2D);
		}

		float SizeSquared2D() const
		{
			return (X * X + Y * Y);
		}

		float& Component(int32_t Index)
		{
			return (&X)[Index];
		}

		float Component(int32_t Index) const
		{
			return (&X)[Index];
		}

		inline float Distance(Vector3D v)
		{
			return (float)std::sqrtf(std::powf(v.X - this->X, 2.0) + powf(v.Y - this->Y, 2.0) + powf(v.Z - this->Z, 2.0));
		}

		inline Vector3D operator *= (Vector3D r)
		{
			this->X *= r.X;
			this->Y *= r.Y;
			this->Z *= r.Z;

			return *this;
		}

		inline Vector3D operator *= (float r)
		{
			this->X *= r;
			this->Y *= r;
			this->Z *= r;

			return *this;
		}

		inline Vector3D operator * (float r)
		{
			this->X = this->X * r;
			this->Y = this->Y * r;
			this->Z = this->Z * r;

			return *this;
		}
	public:
		__forceinline Vector3D& operator+=(const Vector3D& v)
		{
			X += v.X; Y += v.Y; Z += v.Z; return *this;
		}

		__forceinline Vector3D operator+(const Vector3D& v) const
		{
			return Vector3D(X + v.X, Y + v.Y, Z + v.Z);
		}

		Vector3D& operator=(const Vector3D& Vector)
		{
			X = Vector.X;
			Y = Vector.Y;
			Z = Vector.Z;
			return *this;
		}

		bool operator==(const Vector3D& Vector) const
		{
			return (X == Vector.X &&
				Y == Vector.Y &&
				Z == Vector.Z);
		}

		bool operator!=(const Vector3D& Vector) const
		{
			return (X != Vector.X ||
				Y != Vector.Y ||
				Z != Vector.Z);
		}

		float& operator[](int32_t Index)
		{
			return (&X)[Index];
		}

		float operator[](int32_t Index) const
		{
			return (&X)[Index];
		}


		Vector3D operator-(const Vector3D& Vector) const
		{
			return { X - Vector.X,
					 Y - Vector.Y,
					 Z - Vector.Z };
		}

		Vector3D operator/(const Vector3D& Vector) const
		{
			return { X / Vector.X,
					 Y / Vector.Y,
					 Z / Vector.Z };
		}

		Vector3D operator/(const int Delim) const
		{
			return { X / Delim,
					 Y / Delim,
					 Z / Delim };
		}

		/*inline Rotator ToRotator()
		{
			return Rotator{ X, Y, Z };
		}*/

	public:
		static const Vector3D ZeroVector;

	public:
		float X = 0.f;	// 0x0000
		float Y = 0.f;	// 0x0004
		float Z = 0.f;	// 0x0008
	};
	
	const Vector3D Vector3D::ZeroVector = { 0.f, 0.0f, 0.0f };

	struct Matrix
	{
		union
		{
			struct
			{
				float        _11, _12, _13, _14;
				float        _21, _22, _23, _24;
				float        _31, _32, _33, _34;
				float        _41, _42, _43, _44;
			};

			float m[4][4];
		};

		void MatrixMultiply(Matrix* Result, Matrix* Matrix1, Matrix* Matrix2)
		{
			Result->_11 = Matrix1->_11 * Matrix2->_11 + Matrix1->_12 * Matrix2->_21 + Matrix1->_13 * Matrix2->_31 + Matrix1->_14 * Matrix2->_41;
			Result->_12 = Matrix1->_11 * Matrix2->_12 + Matrix1->_12 * Matrix2->_22 + Matrix1->_13 * Matrix2->_32 + Matrix1->_14 * Matrix2->_42;
			Result->_13 = Matrix1->_11 * Matrix2->_13 + Matrix1->_12 * Matrix2->_23 + Matrix1->_13 * Matrix2->_33 + Matrix1->_14 * Matrix2->_43;
			Result->_14 = Matrix1->_11 * Matrix2->_14 + Matrix1->_12 * Matrix2->_24 + Matrix1->_13 * Matrix2->_34 + Matrix1->_14 * Matrix2->_44;

			Result->_21 = Matrix1->_21 * Matrix2->_11 + Matrix1->_22 * Matrix2->_21 + Matrix1->_23 * Matrix2->_31 + Matrix1->_24 * Matrix2->_41;
			Result->_22 = Matrix1->_21 * Matrix2->_12 + Matrix1->_22 * Matrix2->_22 + Matrix1->_23 * Matrix2->_32 + Matrix1->_24 * Matrix2->_42;
			Result->_23 = Matrix1->_21 * Matrix2->_13 + Matrix1->_22 * Matrix2->_23 + Matrix1->_23 * Matrix2->_33 + Matrix1->_24 * Matrix2->_43;
			Result->_24 = Matrix1->_21 * Matrix2->_14 + Matrix1->_22 * Matrix2->_24 + Matrix1->_23 * Matrix2->_34 + Matrix1->_24 * Matrix2->_44;

			Result->_31 = Matrix1->_31 * Matrix2->_11 + Matrix1->_32 * Matrix2->_21 + Matrix1->_33 * Matrix2->_31 + Matrix1->_34 * Matrix2->_41;
			Result->_32 = Matrix1->_31 * Matrix2->_12 + Matrix1->_32 * Matrix2->_22 + Matrix1->_33 * Matrix2->_32 + Matrix1->_34 * Matrix2->_42;
			Result->_33 = Matrix1->_31 * Matrix2->_13 + Matrix1->_32 * Matrix2->_23 + Matrix1->_33 * Matrix2->_33 + Matrix1->_34 * Matrix2->_43;
			Result->_34 = Matrix1->_31 * Matrix2->_14 + Matrix1->_32 * Matrix2->_24 + Matrix1->_33 * Matrix2->_34 + Matrix1->_34 * Matrix2->_44;

			Result->_41 = Matrix1->_41 * Matrix2->_11 + Matrix1->_42 * Matrix2->_21 + Matrix1->_43 * Matrix2->_31 + Matrix1->_44 * Matrix2->_41;
			Result->_42 = Matrix1->_41 * Matrix2->_12 + Matrix1->_42 * Matrix2->_22 + Matrix1->_43 * Matrix2->_32 + Matrix1->_44 * Matrix2->_42;
			Result->_43 = Matrix1->_41 * Matrix2->_13 + Matrix1->_42 * Matrix2->_23 + Matrix1->_43 * Matrix2->_33 + Matrix1->_44 * Matrix2->_43;
			Result->_44 = Matrix1->_41 * Matrix2->_14 + Matrix1->_42 * Matrix2->_24 + Matrix1->_43 * Matrix2->_34 + Matrix1->_44 * Matrix2->_44;
		}

		inline Matrix operator*(Matrix Other)
		{
			Matrix Result;
			MatrixMultiply(&Result, this, &Other);
			return Result;
		};


	};

	struct matrix4x4 // Polnyi Privod
	{
		float m[12];
	};
}