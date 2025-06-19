set nocompatible
syntax on
filetype on
filetype plugin on
filetype indent on

set autoread
set number
set autoindent
set smartindent
set expandtab "turn tabs into spaces
set scrolloff=5
set showmode
set showcmd
set history=1000
"set showmatch
set wildmenu

" Search
set incsearch
set ignorecase
set smartcase
" set hlsearch

"alias semicolon, for easier typing
map ; :

"set shiftwidth=4

"set copyindent
"set shiftwidth=2
"function! ToggleSyntax()
"endfunction
"nnoremap <C-F5>  :call ToggleList()         <cr>
set statusline=

if version >= 703
    set undodir=~/.vim/backup
    set undofile
    set undoreload=10000
endif

" Experimental status line
set statusline=\ %F\ %M\ %Y\ %R\ %=\ ascii:\ %b\ hex:\ 0x%B\ pos:\ %l,%c\ percent:\ %p%%
set laststatus=2

noremap <c-up> <c-w>+
noremap <c-down> <c-w>-
noremap <c-left> <c-w>>
noremap <c-right> <c-w><

let mapleader = ","

" Kill-ring yank.
" First paste with <leader>p.
" Then cycle to older pastes with F1.
nnoremap <leader>p "1p
nnoremap <leader>P "1p
nnoremap <leader>e :find<space>
nnoremap <F1> u.

" for regex??
" set magic
set encoding=utf8
set ffs=unix,dos,mac

" quick save
nnoremap <leader>w :w<cr>



" Return to last edit position when opening files (You want this!)
au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | endif

" set statusline=\ %F%m%r%h\ %w\ \ CWD:\ %r%{getcwd()}%h\ \ \ Line:\ %l\ \ Column:\ %c

set textwidth=80
autocmd FileType rust setlocal textwidth=80
