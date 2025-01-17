/* === S Y N F I G ========================================================= */
/*!	\file value.cpp
**	\brief Template Header
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007, 2008 Chris Moore
**  Copyright (c) 2011 Carlos López
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "value.h"
#include "general.h"
#include <synfig/localization.h>
#include "canvas.h"
#include "valuenodes/valuenode_bone.h"
#include "gradient.h"
#include "matrix.h"
#include "transformation.h"



#include "vector.h"
#include "time.h"
#include "segment.h"
#include "color.h"

#endif

using namespace synfig;
using namespace etl;

/* === M A C R O S ========================================================= */

#define TRY_FIX_FOR_BUG_27

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

ValueBase::ValueBase():
	type(&type_nil),data(nullptr),ref_count(0),loop_(0),static_(0),interpolation_(INTERPOLATION_UNDEFINED)
{
#ifdef INITIALIZE_TYPE_BEFORE_USE
	type->initialize();
#endif
}

ValueBase::ValueBase(Type &x):
	type(&type_nil),data(nullptr),ref_count(0),loop_(0),static_(0),interpolation_(INTERPOLATION_UNDEFINED)
{
#ifdef INITIALIZE_TYPE_BEFORE_USE
	type->initialize();
#endif
	create(x);
}

ValueBase::ValueBase(const ValueBase& x)
	: ValueBase(*x.type)
{
	if(data != x.data)
	{
		Operation::CopyFunc copy_func =
			Type::get_operation<Operation::CopyFunc>(
				Operation::Description::get_copy(type->identifier, type->identifier) );
		if (copy_func)
		{
			copy_func(data, x.data);
		}
		else
		{
			data = x.data;
			ref_count = x.ref_count;
		}
	}

	loop_ = x.loop_;
	static_ = x.static_;
	interpolation_ = x.interpolation_;
}

ValueBase::ValueBase(ValueBase&& x) noexcept
	: ValueBase()
{
	swap(*this, x);
}

ValueBase::~ValueBase()
{
	clear();
}

ValueBase&
ValueBase::operator=(ValueBase x)
{
	swap(*this, x);
	return *this;
}

String
ValueBase::get_string() const
{
	Operation::ToStringFunc func =
		Type::get_operation<Operation::ToStringFunc>(
			Operation::Description::get_to_string(type->identifier) );
	return func ? func(data) : "Invalid type";
}

bool
ValueBase::is_valid()const
{
	return type != &type_nil && ref_count;
}

void
ValueBase::create(Type &type)
{
#ifdef INITIALIZE_TYPE_BEFORE_USE
	type.initialize();
#endif
	if (type == type_nil) { clear(); return; }
	Operation::CreateFunc func =
		Type::get_operation<Operation::CreateFunc>(
			Operation::Description::get_create(type.identifier) );
	assert(func != NULL);
	clear();
	this->type = &type;
	data = func();
	ref_count.reset();
}

void
ValueBase::copy(const ValueBase& x)
{
	Operation::CopyFunc func =
		Type::get_operation<Operation::CopyFunc>(
			Operation::Description::get_copy(type->identifier, x.type->identifier));
	if (func)
	{
		if (!ref_count.unique()) create();
		func(data, x.data);
	}
	else
	{
		Operation::CopyFunc func =
			Type::get_operation<Operation::CopyFunc>(
				Operation::Description::get_copy(x.type->identifier, x.type->identifier));
		if (func)
		{
			if (!ref_count.unique()) create(*x.type);
			func(data, x.data);
		}
	}
	copy_properties_of(x);
}

void
ValueBase::copy_properties_of(const ValueBase& x)
{
	loop_=x.loop_;
	static_=x.static_;
	interpolation_=x.interpolation_;
}

bool
ValueBase::empty()const
{
	return !is_valid() || (type == &type_list ? get_list().empty() : false);
}

Type&
ValueBase::get_contained_type()const
{
	if (type != &type_list || empty())
		return type_nil;
	return get_list().front().get_type();
}

void
ValueBase::clear()
{
	if(ref_count.unique() && data)
	{
		Operation::DestroyFunc func =
			Type::get_operation<Operation::DestroyFunc>(
				Operation::Description::get_destroy(type->identifier) );
		assert(func != NULL);
		func(data);
	}
	ref_count.detach();
	data=nullptr;
	type=&type_nil;
}

Type&
ValueBase::ident_type(const String &str)
{
	Type *type = Type::try_get_type_by_name(str);
	return type ? *type : type_nil;
}

bool
ValueBase::operator==(const ValueBase& rhs)const
{
	Operation::EqualFunc func =
		Type::get_operation<Operation::EqualFunc>(
			Operation::Description::get_equal(type->identifier, rhs.type->identifier) );
	return !func ? false : func(data, rhs.data);
}

bool
ValueBase::operator<(const ValueBase& rhs)const
{
	Operation::LessFunc func =
		Type::get_operation<Operation::LessFunc>(
			Operation::Description::get_less(type->identifier, rhs.type->identifier) );
	return !func ? false : func(data, rhs.data);
}
