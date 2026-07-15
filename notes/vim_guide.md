1. Quickfix list for grep search replace
    - ```grep <result>```
    - Ctrl-q when in telescope
    - ```:cdo %s/<regex>/<replace>/gc``` (c = ask for confirm)
    - ```:ccl``` to close quickfix list

2. Avoid needing to escape regex expressions
    - ```%s/\v<regex>/<replace>/gc```
    - ```\v``` does stuff to avoid escaping regex (),[], etc...