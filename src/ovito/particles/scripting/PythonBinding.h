////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/pyscript/PyScript.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/particles/import/InputColumnMapping.h>
#include <ovito/particles/export/OutputColumnMapping.h>
#include <ovito/stdobj/scripting/PythonBinding.h>

namespace pybind11 { namespace detail {

	/// Automatic Python string list <--> InputColumnMapping conversion
    template<> struct type_caster<Ovito::Particles::InputColumnMapping> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::InputColumnMapping, _("InputColumnMapping"));

        bool load(handle src, bool) {
			try {
				if(!isinstance<sequence>(src)) return false;
				sequence seq = reinterpret_borrow<sequence>(src);
				value.resize(seq.size());
				for(size_t i = 0; i < value.size(); i++) {
					Ovito::Particles::ParticlePropertyReference pref = seq[i].cast<Ovito::Particles::ParticlePropertyReference>();
					if(!pref.isNull()) {
						if(pref.type() != Ovito::Particles::ParticlesObject::UserProperty)
							value[i].mapStandardColumn((Ovito::Particles::ParticlesObject::Type)pref.type(), pref.vectorComponent());
						else
							value[i].mapCustomColumn(pref.name(), qMetaTypeId<Ovito::FloatType>(), pref.vectorComponent());
					}
				}
				return true;
			}
			catch(const cast_error&) {}
			return false;
        }

        static handle cast(const Ovito::Particles::InputColumnMapping& src, return_value_policy /* policy */, handle /* parent */) {
        	list ls;
			for(const auto& col : src)
				ls.append(pybind11::cast(col.property.nameWithComponent()));
			return ls.release();
        }
    };

	/// Automatic Python string list <--> OutputColumnMapping conversion
    template<> struct type_caster<Ovito::Particles::OutputColumnMapping> {
    public:
        PYBIND11_TYPE_CASTER(Ovito::Particles::OutputColumnMapping, _("OutputColumnMapping"));

        bool load(handle src, bool) {
			try {
				if(!isinstance<sequence>(src)) return false;
				sequence seq = reinterpret_borrow<sequence>(src);
				value.reserve(seq.size());
				for(size_t i = 0; i < seq.size(); i++) {
					value.push_back(seq[i].cast<Ovito::Particles::ParticlePropertyReference>());
				}
				return true;
			}
			catch(const cast_error&) {}
			return false;
        }

        static handle cast(const Ovito::Particles::OutputColumnMapping& src, return_value_policy /* policy */, handle /* parent */) {
        	list ls;
			for(const auto& col : src)
				ls.append(pybind11::cast(col));
			return ls.release();
        }
    };

}} // namespace pybind11::detail


