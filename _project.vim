autocmd BufEnter *.c command! -buffer -nargs=* Run !./do-after <args>
autocmd BufEnter *.c set foldmethod=marker
