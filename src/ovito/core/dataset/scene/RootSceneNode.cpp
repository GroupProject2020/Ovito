////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(RootSceneNode);

/******************************************************************************
* Default constructor.
******************************************************************************/
RootSceneNode::RootSceneNode(DataSet* dataset) : SceneNode(dataset)
{
	setNodeName("Scene");

	// The root node does not need a transformation controller.
	setTransformationController(nullptr);
}

/******************************************************************************
* Searches the scene for a node with the given name.
******************************************************************************/
SceneNode* RootSceneNode::getNodeByName(const QString& nodeName) const
{
	SceneNode* result = nullptr;
	visitChildren([nodeName, &result](SceneNode* node) -> bool {
		if(node->nodeName() == nodeName) {
			result = node;
			return false;
		}
		return true;
	});
	return result;
}

/******************************************************************************
* Generates a name for a node that is unique throughout the scene.
******************************************************************************/
QString RootSceneNode::makeNameUnique(QString baseName) const
{
	// Remove any existing digits from end of base name.
	if(baseName.size() > 2 &&
		baseName.at(baseName.size()-1).isDigit() && baseName.at(baseName.size()-2).isDigit())
		baseName.chop(2);

	// Keep appending different numbers until we arrive at a unique name.
	for(int i = 1; ; i++) {
		QString newName = baseName + QString::number(i).rightJustified(2, '0');
		if(getNodeByName(newName) == nullptr)
			return newName;
	}
}

}	// End of namespace
