#!/bin/bash

astyle --suffix=none --brackets=linux --indent=spaces=4 --indent-preprocessor --min-conditional-indent=0 --max-instatement-indent=40 --pad-oper --unpad-paren  --pad-header --fill-empty-lines --align-pointer=name --align-reference=name --convert-tabs --lineend=linux --formatted `cat analog_signature_analyzer.pro | grep -Eoh "\s\S*\.(cpp|h)"`

