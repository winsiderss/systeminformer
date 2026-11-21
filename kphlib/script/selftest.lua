--
-- Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
--
-- This file is part of System Informer.
--
-- Authors:
--
--     jxy-s   2026
--

--
-- The self test of the script engine. Its routines touch nothing.
--
-- A routine receives the input of the call as one string and returns a status
-- and, optionally, the output as one string. The routines a client can call
-- are the ones kphscript.xml lists for this script.
--

-- Returns its input.
function echo(input)
  return ksi.STATUS_SUCCESS, input
end

-- Returns the sum of a list of 64-bit integers, as 8 bytes.
function sum(input)
  local total = 0

  if #input % 8 ~= 0 then
    return ksi.STATUS_INVALID_PARAMETER
  end

  for at = 1, #input, 8 do
    total = total + string.unpack('<i8', input, at)
  end

  return ksi.STATUS_SUCCESS, string.pack('<i8', total)
end
