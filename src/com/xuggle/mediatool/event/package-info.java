/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Events that can be fired by the {@link com.xuggle.mediatool} package.
 * 
 * {@link com.xuggle.mediatool.event.IEvent} is the top of the interface
 * inheritance tree. If a given interface is instantiable as an event, you will
 * find a class with the same name without the starting &quot;I&quot;
 * <p>
 * Mixin classes (e.g. {@link com.xuggle.mediatool.event.AEventMixin}) are
 * provided as abstract classes implementing all the methods implied by their
 * name, but not actually declaring the interface. In this way child classes can
 * extend them, but separately decide which functionality to admit they have to
 * the outside world.
 * </p>
 */
package com.xuggle.mediatool.event;