/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

/**
 * A collection of classes that ferry objects from Java to native code and back,
 * and manage native memory.
 * <p>
 * This package contains a variety of classes used by Xuggler to
 * manage communication between Java and and Native Code.
 * </p>
 * <p>
 * Classes and methods marked &quot;Internal Only&quot; are not meant for usage
 * by anyone outside these libraries. They are public members only because they
 * need to be for other Xuggler packages to use them -- don't go calling methods
 * on these objects from outside Xuggler as you can quickly bring down the Java
 * virtual machine if you don't know what you're doing.
 * </p>
 * <h2>Tuning Ferry (And Xuggler) Memory Management</h2>
 * 
 * <p>
 * A complicated subject, but if you're ready for it, take a look at the
 * {@link com.xuggle.ferry.JNIMemoryManager} object and start reading.
 * </p>
 */
package com.xuggle.ferry;

