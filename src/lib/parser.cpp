module;

export module parser;

import rdparser;
import ll1parser;
import slr1parser;

export const Parser parser =
    RecursiveDescentParser();
