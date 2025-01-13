# setools
convery cil policy to te policy, useful when build twrp
assuming use x86_64 ubuntu. you might need rebuild c files if it couldn't run. 
python script is universial
when this case, build  c file yourself  via gcc <what_you_want_to_build.c> -o <where/what_you_want_to_build>

Help by copilot & tongyi. 


[ trans.py ]translate cil back to te files (kinda undo m4)
usage：python trans.py <file>

[ duplicate_rules_handler_and_sort ] handle duplicate type and attribute in single file
usage: ./duplicate_rules_handler_and_sort <input_file.te> <output_file.te>

[not_defined_checker] check if some type or attribute not defined. delete all your neverallow in your custom te first.
usage： ./not_defined_checker [-t] <custome_te.te> [optional_conf]
-t for test use. it print some debug message including what type/attribute aadded. 
it also could use your conf files to detect exist type define or attribute define in existing project.







link:
https://github.com/u0-ani-nya/setools
