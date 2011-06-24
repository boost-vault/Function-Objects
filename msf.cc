#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/inherit.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <typeinfo>
#include <algorithm>
#include <iostream>
namespace mpl = boost::mpl;

#define MSF_MAX_ARITY 10
#define MSF_MAX_SIGNATURES 40

template<typename Sig>
struct sig_dispatcher;

#define MSF_signature_args(n, z)                \
    BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, T, arg)

#define MSF_template_args(n, z)                 \
    BOOST_PP_ENUM_PARAMS_Z(z, n, class T)

#define MSF_args(n, z)                          \
    BOOST_PP_ENUM_PARAMS_Z(z, n, arg)

#define MSF_make_sig_dispatcher(z, n, data)                                             \
    template<typename R BOOST_PP_COMMA_IF(n) MSF_template_args(n,z)>                    \
    struct sig_dispatcher<R(MSF_signature_args(n,z))> {                                 \
        typedef R(*type)(void * BOOST_PP_COMMA_IF(n) MSF_signature_args(n,z));          \
                                                                                        \
        template<typename F>                                                            \
        struct apply {                                                                  \
            static R value(void* f BOOST_PP_COMMA_IF(n) MSF_signature_args(n,z)) {      \
                return (*static_cast<F*>(f))(MSF_args(n,z));                            \
            }                                                                           \
        };                                                                              \
    };                                                                                  \
    /**/

BOOST_PP_REPEAT(MSF_MAX_ARITY, MSF_make_sig_dispatcher, ~)

struct copy_tag {};
template<>
struct sig_dispatcher<void*(copy_tag)> {
    typedef void*(*type)(void*, copy_tag) ;
    template<typename F>
    struct apply {
        static void* value(void * f, copy_tag) {
            return new F(*static_cast<F*>(f));
        }
    };
};

struct delete_tag{};
template<>
struct sig_dispatcher<void(delete_tag)> {
    typedef void(*type)(void*, delete_tag) ;
    template<typename F>
    struct apply {
        static void value(void * f, delete_tag) {
            delete static_cast<F*>(f);
        }
    };
};

struct typeid_tag{};
template<>
struct sig_dispatcher<const std::type_info&(typeid_tag)> {
    typedef const std::type_info&(*type)(void*, typeid_tag) ;
    template<typename F>
    struct apply {
        static const std::type_info& value(void * f, typeid_tag) {
            return typeid(*static_cast<F*>(f));
        }
    };
};

namespace detail {
    template<typename T1>
    bool operator==(const T1&, const T1&) {
        return false;
    }
}

// We can't enable this by default for the same reason it cannot be
// enabled for boost function: the existence of operator== cannot be
// guaranteed;
struct compare_same_type_tag{};
template<>
struct sig_dispatcher<bool(void*, compare_same_type_tag)> {
    typedef bool(*type)(void*, void*, compare_same_type_tag) ;
    template<typename F>
    struct apply {
        // this function assumes that f and g have the same typeid
        static bool value(void * f, void * g, compare_same_type_tag) {
            using detail::operator==;
            return (*static_cast<F*>(f)) == (*static_cast<F*>(g));
        }
    };
};

template<typename F, typename Type, typename Tag>
struct get_value {
    enum {value = 10};
};

template<typename T, typename Tag>
struct tagged_type {};

// we can also put values in the vtable. Currently unused. we could
// put the typeid of T here, but the result of typeid() is
// unfortunately not a constant expression
template<typename Type, typename Tag>
struct sig_dispatcher<tagged_type<Type, Tag> > {
    typedef int type;
    template<typename F>
    struct apply {
        enum { value = get_value<F, Type, Tag>::value };
    };
};

#define MSF_vtable_entry(z, n, v)                                                                       \
    typedef sig_dispatcher<typename mpl::at_c<v, n>::type> sig_dispatcher ## n ;                        \
    typedef typename sig_dispatcher ## n::type entry_ ## n ## _type;                                    \
    const entry_ ## n ## _type  entry_ ## n;                                                            \
    const entry_ ## n ## _type  at(typename mpl::at_c<v, n>::type*) const { return entry_ ## n; }       \
    /**/
    

#define MSF_make_vtable_impl_of_size(z, n, unused)              \
    template<class SigVector>                                   \
    struct vtable_impl ## n {                                   \
        BOOST_PP_REPEAT_ ## z(n, MSF_vtable_entry, SigVector)   \
    };                                                          \
    /**/

#define MSF_fill_vtable_element(z, n, f)                        \
    type :: sig_dispatcher ##n :: template apply<f>::value      \
    /**/

template<typename SigVector, int N = mpl::size<SigVector>::value >
struct as_vtable;

#define MSF_specialize_as_vtable(z, n, unused)                          \
    MSF_make_vtable_impl_of_size(z, n, ~)                               \
                                                                        \
    template<typename SigVector>                                        \
    struct as_vtable<SigVector, n> {                                    \
        typedef const vtable_impl ## n<SigVector> type ;                \
                                                                        \
        template<typename F>                                            \
        static type* get_vtable() {                                     \
            static type vtable = {                                      \
                BOOST_PP_ENUM_ ## z( n, MSF_fill_vtable_element, F)     \
            };                                                          \
            return &vtable;                                             \
        }                                                               \
    };                                                                  \
    /**/


BOOST_PP_REPEAT_FROM_TO(1, MSF_MAX_SIGNATURES, MSF_specialize_as_vtable, ~)

struct decltype_tag {};

template<typename Derived, typename Sig, typename N>
struct base {
    struct cannot_call_me;
 public:
    void operator()(cannot_call_me&){}
};

class terminator {
    struct cannot_call_me;
 public:
    void operator()(cannot_call_me&){}
};

#define MSF_specialize_base(z, n, unused)                                                                                                       \
    template<class Derived, class Result BOOST_PP_COMMA_IF(n) MSF_template_args(n, z), typename  N>                                             \
    struct base<Derived, Result(MSF_signature_args(n, z)), N> {                                                                                 \
        Result operator()( MSF_signature_args(n, z) ){                                                                                          \
            Derived& derived = static_cast<Derived&>(*this);                                                                                    \
            return derived.vtablep->at(static_cast<Result(*)(MSF_signature_args(n,z))>(0))(derived.p BOOST_PP_COMMA_IF(n) MSF_args(n, z));      \
        }                                                                                                                                       \
        typedef char decltype_result[N::value]  ;                                                                                               \
        decltype_result& operator()(decltype_tag BOOST_PP_COMMA_IF(n) MSF_signature_args(n, z) );                                               \
    };                                                                                                                                          \
    /**/


BOOST_PP_REPEAT(MSF_MAX_SIGNATURES, MSF_specialize_base, ~);
            
template<typename SigVector>
struct as_extended_sig_vector
    : mpl::apply<
          mpl::push_back<
              mpl::push_back<
                  mpl::push_back<
                      mpl::push_back<mpl::_1, void*(copy_tag)>
                    , void(delete_tag)
                  >
                , bool(void*, compare_same_type_tag)
              >
            , const std::type_info&(typeid_tag)
          >
        , SigVector
      > {};

template<typename Derived, typename V, typename  N>
struct inherit
    : base<Derived, typename mpl::front<V>::type, N>
    , mpl::apply<
          mpl::if_<
              mpl::greater<
                  mpl::size<mpl::_1>
                , mpl::int_<1>
              >
            , inherit<
                  Derived
                , mpl::pop_front<mpl::_1>
                , mpl::plus<N, mpl::int_<1> > 
              >
            , terminator
          >
        , V
      >::type {
    using base<Derived, typename mpl::front<V>::type, N>::operator();

    using
    mpl::apply<
        mpl::if_<
            mpl::greater<
                mpl::size< mpl::_1  >
              , mpl::int_<1>
            >
          , inherit< Derived, mpl::pop_front<mpl::_1>, mpl::plus<N, mpl::int_<1> > > , terminator>, V>::type 
    ::operator();  
};

template<typename T>
T make();

template<typename Sigv, int index>
struct result_at_index {
    typedef typename
    boost::function_traits<
        typename mpl::at_c<Sigv, index>::type
    >::result_type type;
};

template<typename SigVector>
class msf
    : public inherit< msf<SigVector>, SigVector, mpl::int_<1> > {
    template<typename Derived, typename Sig, typename  N>
    friend struct base;
    
    typedef typename as_extended_sig_vector<SigVector>::type extended_sig_vector;
    
    typedef typename as_vtable<extended_sig_vector>::type vtable;
    vtable * vtablep;
    void * p;
 public:
    
    using inherit<msf<SigVector>, SigVector, mpl::int_<1> >::operator();

    // The only sane way to implement this is decltype. With the
    // lacking of it, we can use sizeof as our decltype substitute, by
    // using an extra overload of each operator() that returns a
    // reference to char array of the size equal to the index in the
    // sigvector sequence (plus one actually).
    template<typename Sig>
    struct result;

#define MSF_generate_make_param(z, n, unused)   \
    make<T ## n>()                              \
    /**/
    
#define MSF_generate_result(z, n, unused)                                                       \
    template<typename R BOOST_PP_COMMA_IF(n) MSF_template_args(n, z)>                           \
    struct result<R(MSF_signature_args(n, z))> {                                                \
        enum { index                                                                            \
               = sizeof                                                                         \
               (make<msf<SigVector> >()                                                         \
                (decltype_tag() BOOST_PP_ENUM_TRAILING_ ## z(n, MSF_generate_make_param, ~))    \
               ) -1                                                                             \
        };                                                                                      \
        typedef typename result_at_index<extended_sig_vector, index>::type type;                \
    };                                                                                          \
    /**/
    
    BOOST_PP_REPEAT(MSF_MAX_ARITY, MSF_generate_result, ~)
        
    template<typename F>
    explicit msf(F const& f)
    : vtablep(as_vtable<extended_sig_vector>::template get_vtable<F>())
        , p(new F(f))
    { };

    msf()
        : vtablep(0)
        , p(0) {}
    
    msf(msf const& rhs)
        : vtablep(rhs.vtablep)
        , p(vtablep->at(static_cast<void*(*)(copy_tag)>(0))(rhs.p, copy_tag()))
    {}

    void swap(msf& rhs) {
        std::swap(rhs.vtablep, vtablep);
        std::swap(rhs.p, p);
    }

    friend void swap(msf& lhs, msf& rhs) {
        lhs.swap(rhs);
    }
    
    msf& operator=(msf rhs) {
        swap(rhs);
        return *this;
    }

    template<typename T2>
    msf& operator=(const T2& f) {
        msf (f).swap(*this);
        return *this;
    }
    
    ~msf() {
        if(vtablep)
            vtablep->at(static_cast<void(*)(delete_tag)>(0))(p, delete_tag());
    }

};



struct foo {

    template<typename Op>
    Op operator()(Op) const{
        std::cout << "foo::operator("<<typeid(Op).name()<<")\n";
    };

    float operator()(double) const{
        std::cout << "foo::operator(double, double)\n";
    };
    
    void operator() () const {
        std::cout << "foo::operator()\n";
    }

    ~foo() {
        std::cout << "~foo()\n";
    }

    foo() {
        std::cout << "foo()\n";
    }

    foo(foo const&) {
        std::cout << "foo(const foo&)\n";
    }
    //private:
    friend
    bool operator==(foo, foo) {
    }
};

    
struct my_tag;
struct my_struct {};
int main() {
    foo my_foo;
    typedef msf<mpl::vector<my_struct(my_struct), int(int), float (double) , float (float), short (short), char(char), tagged_type<int, my_tag>  > > my_msf;
    my_msf f ( my_foo );
    std::cout << typeid(boost::result_of<my_msf(my_struct)>::type).name()<<std::endl;
    std::cout << typeid(boost::result_of<my_msf(double)>::type).name()<<std::endl;
    //f = foo();
    //f();
    f(10.0);
    f(10);
}
