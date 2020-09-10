(add-hook 'c-mode-hook 'lsp-development-mode)
(add-hook 'c++-mode-hook 'lsp-development-mode)

(defun lsp-development-mode ()
  "Set up LSP stuff"
  (interactive)
  (lsp t)
  (setq show-trailing-whitespace t)
  (setq indent-tabs-mode nil))
