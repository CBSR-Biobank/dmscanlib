﻿/*
libdmtx-net - .NET wrapper for libdmtx

Copyright (C) 2009 Joseph Ferner / Tom Vali

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: libdmtx@fernsroth.com
*/

/* $Id: TestRunner.cs 688 2009-02-12 17:25:32Z joegtp $ */

using System;

namespace Libdmtx {
    public class TestRunner {
        public static void Main(string[] args) {
            string assemblyFileName = typeof(TestRunner).Assembly.CodeBase;
            assemblyFileName = assemblyFileName.Substring("file:///".Length);
            string[] nunitArgs = new[] { assemblyFileName };
            NUnit.ConsoleRunner.Runner.Main(nunitArgs);
            Console.WriteLine("Press any key to continue");
            Console.ReadKey();
        }
    }
}
