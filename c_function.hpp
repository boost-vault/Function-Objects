//~ Copyright (c) 2005-2007 Redshift Software, Inc.
//~ Distributed under the Boost Software License, Version 1.0. 
//~ (See accompanying file LICENSE_1_0.txt or copy at 
//~ http://www.boost.org/LICENSE_1_0.txt)

#ifndef com_redshift_software_libraries_base_c_function_hpp
#define com_redshift_software_libraries_base_c_function_hpp

#include <boost/function.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/type_traits/function_traits.hpp>


namespace redshift { namespace base {
	
	#ifndef RSI_BASE_MAKE_C_FUNCTION_MAX_ARITY
		#define RSI_BASE_MAKE_C_FUNCTION_MAX_ARITY 9
	#endif
	
	namespace detail
	{
		template <typename InterfaceGUID, typename FunctionType>
		struct c_function_traits
		{
			typedef typename boost::remove_pointer<FunctionType>::type
				function_type;
			typedef boost::function<function_type>
				function_shunt;
			typedef boost::function_traits<function_type>
				function_traits;
			static const int
				arity = function_traits::arity;
			typedef typename function_traits::result_type
				result_type;
		};
		
		template <int Arity> struct c_function { };
		
		#define RSI_LIB_BASE_C_FUNCTION(z,ARITY,d) \
		template <> struct c_function<ARITY> \
		{ \
			template <typename InterfaceGUID, typename FunctionType> \
			struct instance \
			{ \
				typedef instance<InterfaceGUID,FunctionType> \
					self_type; \
				typedef c_function_traits<InterfaceGUID,FunctionType> \
					traits_type; \
				typedef typename traits_type::function_traits F; \
				static \
					typename traits_type::function_shunt & shunt() \
				{ \
					static typename traits_type::function_shunt shunt_; \
					return shunt_; \
				} \
				static \
					typename traits_type::result_type \
					function ( BOOST_PP_ENUM(ARITY,RSI_LIB_BASE_C_FUNCTION_ARGN,_) ) \
				{ \
					return (self_type::shunt()) ( \
						BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(ARITY),a) \
						); \
				} \
			}; \
		};
		#define RSI_LIB_BASE_C_FUNCTION_ARGN(x,n,d) \
			BOOST_PP_CAT(BOOST_PP_CAT(typename F::arg,BOOST_PP_INC(n)),_type) \
				BOOST_PP_CAT(a,BOOST_PP_INC(n))
		BOOST_PP_REPEAT(RSI_BASE_MAKE_C_FUNCTION_MAX_ARITY,RSI_LIB_BASE_C_FUNCTION,_)
		#undef RSI_LIB_BASE_C_FUNCTION
		#undef RSI_LIB_BASE_C_FUNCTION_ARGN
	}

	/**
		Returns a pointer to a C function that will call the given function
		object when called. Given the possibility that one could create multiple
		C functions to a single target function object type make_c_function
		takes as the first template argument a type identifier to distinguish
		between them. The second template argument specifies the type of
		the C function created. Example:
		
			class A
			{
				void some_call(int);
			};
			A a;
			void (* cf)(int)
				= make_c_function<struct call_0, void(*)(int)>(
					boost::bind(A::some_call,&a,boost::_1)
					);
	*/
	template <
		typename InterfaceGUID,
		typename FunctionType,
		typename CallType
		>
		typename
			detail::c_function_traits<InterfaceGUID,FunctionType>::function_type *
				make_c_function(CallType call)
	{
		typedef detail::c_function_traits<InterfaceGUID,FunctionType> cf;
		typedef
			typename detail::c_function<cf::arity>
				::template instance<InterfaceGUID,FunctionType>
			cfi;
		
		cfi::shunt() = call;
		return &cfi::function;
	}

} }

#ifdef com_redshift_software_libraries_base_p
	com_redshift_software_libraries_base_p {
		using ::redshift::base::make_c_function;
	} p_com_redshift_software_libraries_base
#endif

#endif
