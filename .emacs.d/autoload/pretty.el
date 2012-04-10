; Better handling of multiple buffers with same name.
(require 'uniquify)
(setq uniquify-buffer-name-style 'post-forward)

; Show column number next to row rumber.
(column-number-mode 1)

; Enable narrow mode.
(put 'narrow-to-region 'disabled nil)

; Highlight traling whitespace and tabs.
(require 'whitespace)
(setq whitespace-style '(face empty tabs lines-tail trailing))
(global-whitespace-mode t)
