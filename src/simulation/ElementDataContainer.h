/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ElementDataContainer_h
#define ElementDataContainer_h

#include <memory>
#include "common/tpt-compat.h"

/**
 * Base struct for storing deltas of element data. These deltas are used in the snapshot (ctrl+z) handlers to restore state.
 * Element data containers don't need to define a delta strategy, if they do not then the whole container will be copied using the Clone
 * method. Elements storing a very large amount of data, like PRTI, may choose to use this, but containers with small or simple data
 * do not need to
 */
struct ElementDataContainerDelta
{

};

class Simulation;
class ElementDataContainer
{
public:
	ElementDataContainer() {}
	virtual std::unique_ptr<ElementDataContainer> Clone() = 0;
	virtual ~ElementDataContainer() { }
	virtual void Simulation_Cleared(Simulation *sim) {}
	virtual void Simulation_BeforeUpdate(Simulation *sim) {}
	virtual void Simulation_AfterUpdate(Simulation *sim) {}
	/**
	 * Compare two element data containers and return a delta. ElementDataContainers may opt to override this if they store a lot of data.
	 * The default implementation returns null, which lets the snapshot code know to just take a clone of the whole data container.
	 */
	virtual std::unique_ptr<ElementDataContainerDelta> Compare(const std::unique_ptr<ElementDataContainer> &o, const std::unique_ptr<ElementDataContainer> &n) { return nullptr; }
	virtual void Forward(const std::unique_ptr<ElementDataContainer> &datacontainer, const std::unique_ptr<ElementDataContainerDelta> &delta) {}
	virtual void Restore(const std::unique_ptr<ElementDataContainer> &datacontainer, const std::unique_ptr<ElementDataContainerDelta> &delta) {}
};

#endif
