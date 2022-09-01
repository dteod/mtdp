# Copyright (C) 2021-2022 Domenico Teodonio
# 
# mtdp is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# mtdp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

function(target_hard_compilation tgt scope)
    target_compile_options(${tgt} ${scope}
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -Wall -Wextra -Wpedantic --pedantic-errors -Werror>
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4>
    )
endfunction()
