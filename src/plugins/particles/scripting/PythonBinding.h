///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/import/InputColumnMapping.h>
#include <plugins/particles/export/OutputColumnMapping.h>
#include <plugins/stdobj/scripting/PythonBinding.h>

namespace pybind11 { namespace detail {

	/// Automatic Python string <--> ParticlePropertyReference conversion
    template<> struct type_caster<Ovito::Particles::ParticlePropertyReference> : public typed_property_ref_caster<Ovito::Particles::ParticleProperty> {
	};
	
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
						if(pref.type() != Ovito::Particles::ParticleProperty::UserProperty)
							value[i].mapStandardColumn((Ovito::Particles::ParticleProperty::Type)pref.type(), pref.vectorComponent());
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


