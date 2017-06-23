#pragma once
/**
	@file
	@brief BGN encryption with prime-order groups
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause

	David Mandell Freeman:
	Converting Pairing-Based Cryptosystems from Composite-Order Groups to Prime-Order Groups. EUROCRYPT 2010: 44-61
	http://theory.stanford.edu/~dfreeman/papers/subgroups.pdf

	BGN encryption
	http://theory.stanford.edu/~dfreeman/cs259c-f11/lectures/bgn
*/
#include <vector>
#ifdef MCL_USE_BN384
#include <mcl/bn384.hpp>
#else
#include <mcl/bn256.hpp>
#define MCL_USE_BN256
#endif

namespace mcl { namespace bgn {

namespace local {

template<class G>
int logG(const G& P, const G& xP)
{
	if (xP.isZero()) return 0;
	if (xP == P) return 1;
	G negT;
	G::neg(negT, P);
	if (xP == negT) return -1;
	G T = P;
	for (int i = 2; i < 100; i++) {
		T += P;
		if (xP == T) return i;
		G::neg(negT, T);
		if (xP == negT) return -i;
	}
	throw cybozu::Exception("BGN:logG:not found");
}

} // mcl::bgn::local

template<class BN, class Fr>
struct BGNT {
	typedef typename BN::G1 G1;
	typedef typename BN::G2 G2;
	typedef typename BN::Fp12 GT;

	class SecretKey;
	class PublicKey;
	class CipherText;

	static G1 P;
	static G2 Q;

	static inline void init(const mcl::bn::CurveParam& cp = mcl::bn::CurveFp254BNb)
	{
#ifdef MCL_USE_BN256
		mcl::bn256::bn256init(cp);
#endif
#ifdef MCL_USE_BN384
		mcl::bn384::bn384init(cp);
#endif
		BN::hashAndMapToG1(P, "0");
		BN::hashAndMapToG2(Q, "0");
	}

	class SecretKey {
		Fr x1, y1, z1;
		Fr x2, y2, z2;
		G1 B1; // (x1 y1 - z1) P
		G2 B2; // (x2 y2 - z2) Q
		Fr x1x2;
		GT g; // e(B1, B2)
	public:
		template<class RG>
		void setRand(RG& rg)
		{
			x1.setRand(rg);
			y1.setRand(rg);
			z1.setRand(rg);
			x2.setRand(rg);
			y2.setRand(rg);
			z2.setRand(rg);
			G1::mul(B1, P, x1 * y1 - z1);
			G2::mul(B2, Q, x2 * y2 - z2);
			x1x2 = x1 * x2;
			BN::pairing(g, B1, B2);
		}
		/*
			set (xP, yP, zP) and (xQ, yQ, zQ)
		*/
		void getPublicKey(PublicKey& pub) const
		{
			G1::mul(pub.xP, P, x1);
			G1::mul(pub.yP, P, y1);
			G1::mul(pub.zP, P, z1);
			G2::mul(pub.xQ, Q, x2);
			G2::mul(pub.yQ, Q, y2);
			G2::mul(pub.zQ, Q, z2);
		}
		// log_x(y)
		int logGT(const GT& x, const GT& y) const
		{
			if (y == 1) return 0;
			if (y == x) return 1;
			GT inv;
			BN::unitaryInv(inv, x);
			if (y == inv) return -1;
			GT t = x;
			for (int i = 2; i < 100; i++) {
				t *= x;
				if (y == t) return i;
				BN::unitaryInv(inv, t);
				if (y == inv) return -i;
			}
			throw cybozu::Exception("BGN:dec:logGT:not found");
		}
		int dec(const CipherText& c) const
		{
			if (c.g.empty()) {
				/*
					S = myP + rP
					T = mzP + rxP
					R = xS - T = m(xy - z)P = mB
				*/
				G1 R1;
				G1::mul(R1, c.S1, x1);
				R1 -= c.T1;
				int m1 = local::logG(B1, R1);
#if 1 // for debug
				G2 R2;
				G2::mul(R2, c.S2, x2);
				R2 -= c.T2;
				int m2 = local::logG(B2, R2);
				if (m1 != m2) {
					throw cybozu::Exception("bad dec") << m1 << m2;
				}
#endif
				return m1;
			}
			/*
				(s, t, u, v) := (e(S, S'), e(S, T'), e(T, S'), e(T, T'))
				s^(xx') v / (t^x u^x')
				= e(xS, x'S') e(xS, -T') e(-T, x'S') e(T, T')
				= e(xS - T, x'S' - T')
				= e(m B1, m' B2)
				= e(B1, B2)^(mm')
			*/
			GT s, t, u;
			GT::pow(s, c.g[0], x1x2);
			s *= c.g[3];
			GT::pow(t, c.g[1], x1);
			GT::pow(u, c.g[2], x2);
			t *= u;
			BN::unitaryInv(t, t);
			s *= t;
			return logGT(g, s);
		}
	};

	class PublicKey {
		G1 xP, yP, zP;
		G2 xQ, yQ, zQ;
		friend class SecretKey;
		/*
			(S1, T1) = (m yP + rP, m zP + r xP)
		*/
		template<class G, class RG>
		static void enc1(G& C0, G& C1, const G& P, const G& xP, const G& yP, const G& zP, int m, RG& rg)
		{
			Fr r;
			r.setRand(rg);
			G C;
			G::mul(C0, yP, m);
			G::mul(C, P, r);
			C0 += C;
			G::mul(C1, zP, m);
			G::mul(C, xP, r);
			C1 += C;
		}
	public:
		template<class RG>
		void enc(CipherText& c, int m, RG& rg) const
		{
			enc1(c.S1, c.T1, P, xP, yP, zP, m, rg);
			enc1(c.S2, c.T2, Q, xQ, yQ, zQ, m, rg);
		}
	};

	class CipherText {
		typedef std::vector<GT> GTVec;
		G1 S1, T1;
		G2 S2, T2;
		GTVec g;
		friend class SecretKey;
		friend class PublicKey;
	public:
		static inline void add(CipherText& z, const CipherText& x, const CipherText& y)
		{
			if (x.g.empty() && y.g.empty()) {
				/*
					(S, T) + (S', T') = (S + S', T + T')
				*/
				G1::add(z.S1, x.S1, y.S1);
				G1::add(z.T1, x.T1, y.T1);
				G2::add(z.S2, x.S2, y.S2);
				G2::add(z.T2, x.T2, y.T2);
				return;
			}
			if (!x.g.empty() && !y.g.empty()) {
				/*
					(g[i]) * (g'[i]) = (g[i] * g'[i])
				*/
				for (size_t i = 0; i < z.g.size(); i++) {
					GT::mul(z.g[i], x.g[i], y.g[i]);
				}
				return;
			}
			// QQQ
			throw cybozu::Exception("bgn:CipherText:add:not supported");
		}
		static inline void mul(CipherText& z, const CipherText& x, const CipherText& y)
		{
			if (!x.g.empty() || !y.g.empty()) {
				throw cybozu::Exception("bgn:CipherText:mul:already mul");
			}
			/*
				(S, T) * (S', T') = (e(S, S'), e(S, T'), e(T, S'), e(T, T'))
			*/
			z.g.resize(4);
			BN::pairing(z.g[0], x.S1, y.S2);
			BN::pairing(z.g[1], x.S1, y.T2);
			BN::pairing(z.g[2], x.T1, y.S2);
			BN::pairing(z.g[3], x.T1, y.T2);
		}
		void add(const CipherText& c) { add(*this, *this, c); }
		void mul(const CipherText& c) { mul(*this, *this, c); }
	};
};

template<class BN, class Fr>
typename BN::G1 BGNT<BN, Fr>::P;

template<class BN, class Fr>
typename BN::G2 BGNT<BN, Fr>::Q;

} } // mcl::bgn

