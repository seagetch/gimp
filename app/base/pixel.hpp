#ifndef __PIXEL_HPP__
#define __PIXEL_HPP__

struct Pixel {
  // Currently BPP=8 is assumed.
  typedef guchar  data_t;
  typedef guint32 internal_t;
  
  template<typename T> struct expressions {
    typedef T E_TYPE;
    T e; 
    expressions(const T& src) : e(src) {};
  };
  
  
  struct value_t {
    static const int DEGREE = 1;
    data_t v;
    value_t(const data_t src) : v(src) {}
    expressions<value_t> wrap() const { return expressions<value_t>(*this); }
  };
  template<typename T1, typename T2> struct prod_t {
    static const int DEGREE = T1::E_TYPE::DEGREE + T2::E_TYPE::DEGREE;
    T1 left;
    T2 right;
    prod_t(const T1& l, const T2& r) : left(l), right(r) {}
    expressions<prod_t> wrap() const { return expressions<prod_t>(*this); }
  };
  template<typename T1, typename T2> struct add_t {
    static const int DEGREE = T1::E_TYPE::DEGREE > T2::E_TYPE::DEGREE ? 
      T1::E_TYPE::DEGREE : T2::E_TYPE::DEGREE;
    T1 left;
    T2 right;
    add_t(const T1& l, const T2& r) : left(l), right(r) {}
    expressions<add_t> wrap() const { return expressions<add_t>(*this); }
  };
  template<typename T1, typename T2> struct sub_t {
    static const int DEGREE = T1::E_TYPE::DEGREE > T2::E_TYPE::DEGREE ? 
      T1::E_TYPE::DEGREE : T2::E_TYPE::DEGREE;
    T1 left;
    T2 right;
    sub_t(const T1& l, const T2& r) : left(l), right(r) {}
    expressions<sub_t> wrap() const { return expressions<sub_t>(*this); }
  };
  template<typename T1, typename T2> struct div_t {
    static const int DEGREE = T1::E_TYPE::DEGREE - T2::E_TYPE::DEGREE;
    T1 numer;
    T2 denom;
    div_t(const T1& l, const T2& r) : numer(l), denom(r) {}
    expressions<div_t> wrap() const { return expressions<div_t>(*this); }
  };
  static const int DEPTH = sizeof(data_t) * 8;
  static const data_t MAX_VALUE = (data_t)((internal_t)(1 << DEPTH) - 1);
  
  static data_t from_f(const float v) { 
    return (data_t)(CLAMP(CLAMP(v, 0, 1.0) * MAX_VALUE, 0, MAX_VALUE));
  };
  static float to_f(const data_t v) {
    return ((float)v) / MAX_VALUE;
  };
  static data_t from_u8(const guchar v) {
    return v;
  };
  static guchar to_u8(const data_t v) {
    return v;
  };
  typedef expressions<Pixel::value_t> pixel_t;
};

// x -> P(x)
inline
Pixel::expressions<Pixel::value_t> pix(const Pixel::data_t v) { 
  return Pixel::value_t(v).wrap();
}
// f -> P(_(f))
inline
Pixel::expressions<Pixel::value_t> f2p(const float f) { 
  return Pixel::value_t(Pixel::from_f(f)).wrap();
}

template<typename T1, typename T2>
inline
Pixel::expressions<Pixel::prod_t<Pixel::expressions<T1>, Pixel::expressions<T2> > > 
operator * (const Pixel::expressions<T1>& l,
             const Pixel::expressions<T2>& r) {
  return Pixel::prod_t<Pixel::expressions<T1>, Pixel::expressions<T2> >(l, r).wrap();
}

template<typename T1, typename T2>
inline
Pixel::expressions<Pixel::div_t<Pixel::expressions<T1>, Pixel::expressions<T2> > > 
operator / (const Pixel::expressions<T1>& l,
             const Pixel::expressions<T2>& r) {
  return Pixel::div_t<Pixel::expressions<T1>,Pixel::expressions<T2> >(l, r).wrap();
}

template<typename T1, typename T2>
inline
Pixel::expressions<Pixel::add_t<Pixel::expressions<T1>, Pixel::expressions<T2> > >
operator + (const Pixel::expressions<T1>& l, const Pixel::expressions<T2>& r) {
  return Pixel::add_t<Pixel::expressions<T1>,Pixel::expressions<T2> >(l, r).wrap();
}


template<typename T1, typename T2>
inline
Pixel::expressions<Pixel::sub_t<Pixel::expressions<T1>, Pixel::expressions<T2> > >
operator - (const Pixel::expressions<T1>& l, const Pixel::expressions<T2>& r) {
  return Pixel::sub_t<Pixel::expressions<T1>,Pixel::expressions<T2> >(l, r).wrap();
}

inline
Pixel::internal_t raw(const Pixel::expressions<Pixel::value_t>& c) {
  return (Pixel::internal_t)c.e.v;
}

inline
Pixel::data_t eval(const Pixel::expressions<Pixel::value_t>& c) {
  return (Pixel::data_t)c.e.v;
}

template<typename T1, typename T2>
inline
Pixel::internal_t raw(const Pixel::expressions<Pixel::prod_t<T1, T2> >& t) {
  return raw(t.e.left) * raw(t.e.right);
}

template<typename T1, typename T2>
inline
Pixel::internal_t raw(const Pixel::expressions<Pixel::div_t<T1, T2> >& t) {
  return raw(t.e.numer) / raw(t.e.denom);
}

template<typename T1, typename T2>
inline
Pixel::internal_t raw(const Pixel::expressions<Pixel::add_t<T1, T2> >& t) {
  switch (T1::E_TYPE::DEGREE - T2::E_TYPE::DEGREE) {
  case -3:
    return raw(t.e.left * f2p(1.0) * f2p(1.0) * f2p(1.0)) + raw(t.e.right);
  case -2:
    return raw(t.e.left * f2p(1.0) * f2p(1.0)) + raw(t.e.right);
  case -1:
    return raw(t.e.left * f2p(1.0)) + raw(t.e.right);
  case  0:
    return raw(t.e.left) + raw(t.e.right);
  case  1:
    return raw(t.e.left) + raw(t.e.right * f2p(1.0));
  case  2:
    return raw(t.e.left) + raw(t.e.right * f2p(1.0) * f2p(1.0));
  case  3:
    return raw(t.e.left) + raw(t.e.right * f2p(1.0) * f2p(1.0) * f2p(1.0));
  default:
    throw "unsupported complexity";
  }
}

template<typename T1, typename T2>
inline
Pixel::internal_t raw(const Pixel::expressions<Pixel::sub_t<T1, T2> >& t) {
  switch (T1::E_TYPE::DEGREE - T2::E_TYPE::DEGREE) {
  case -3:
    return raw(t.e.left * f2p(1.0) * f2p(1.0) * f2p(1.0)) - raw(t.e.right);
  case -2:
    return raw(t.e.left * f2p(1.0) * f2p(1.0)) - raw(t.e.right);
  case -1:
    return raw(t.e.left * f2p(1.0)) - raw(t.e.right);
  case  0:
    return raw(t.e.left) - raw(t.e.right);
  case  1:
    return raw(t.e.left) - raw(t.e.right * f2p(1.0));
  case  2:
    return raw(t.e.left) - raw(t.e.right * f2p(1.0) * f2p(1.0));
  case  3:
    return raw(t.e.left) - raw(t.e.right * f2p(1.0) * f2p(1.0) * f2p(1.0));
  default:
    throw "unsupported complexity";
  }
}

template<typename T>
inline
Pixel::data_t eval(const Pixel::expressions<T>& t) {
  const int deg = T::DEGREE;
  switch (deg) {
  case 3:
    return raw(t) / raw(f2p(1.0)*f2p(1.0));
  case 2:
    return raw(t) / raw(f2p(1.0));
  case 1:
    return raw(t);
  case 0:
    return raw(t) * raw(f2p(1.0));
  case -1:
    return raw(t) * raw(f2p(1.0) * f2p(1.0));
  case -2:
    return raw(t) * raw(f2p(1.0) * f2p(1.0) * f2p(1.0));
  default:
    throw "unsupported type";
  }
}
template<typename T1, typename T2>
inline
Pixel::data_t eval(const Pixel::expressions<Pixel::div_t<T1, T2> >& t) {
  const int deg = Pixel::div_t<T1, T2>::DEGREE;
  switch (deg) {
  case 3:
    return raw(t) / raw(f2p(1.0)*f2p(1.0));
  case 2:
    return raw(t) / raw(f2p(1.0));
  case 1:
    return raw(t);
  case 0:
    return raw(t.e.numer) * raw(f2p(1.0)) / raw(t.e.denom);
  case -1:
    return raw(t.e.numer) * raw(f2p(1.0) * f2p(1.0)) / raw(t.e.denom);
  case -2:
    return raw(t.e.numer) * raw(f2p(1.0) * f2p(1.0) * f2p(1.0)) / raw(t.e.denom);
  default:
    throw "unsupported type";
  }
}
#endif
