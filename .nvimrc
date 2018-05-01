function! Formatonsave()
  "let l:formatdiff = 0
  let l:lines = 'all'
  pyf ~/.vim/clang-format.py
endfunction
autocmd BufWritePre *.h,*.cc,*.cpp,*.hpp,*.c call Formatonsave()

set sw=4
set sts=4
