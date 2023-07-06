;; TBMs:
;; variable till $\n — how to define?
;; determine which identifiers (rule name, var name) may contain slashes

(require 'smie)

(defvar test-mode-indent-offset 2)

(defvar test-mode-top-keywords
  '("build" "rule" "default" "subninja" "include" "pool"))

;; (defconst test-mode-smie-grammar
(setq test-mode-smie-grammar
  (smie-prec2->grammar
   (smie-bnf->prec2
    '((id)
      (variable (id "=" string))
      (string ("STRING"))
      (top (("rule") ("build") variable))
      )))
  )

(defun test-mode-smie-rules (kind token)
  (pcase kind
    ;; (`(:after . "build")
    ;;  test-mode-indent-offset)
    ;; (`(:after . "rule")
    ;;  test-mode-indent-offset)
    (:before (if (member token test-mode-top-keywords)
                 0
               test-mode-indent-offset))
    (_ (progn
         ;; (print kind)
         ;; (print token)
         test-mode-indent-offset
         )))
  )

(defun end-of-line-pos ()
  "TBM: integrate in into C?"
  (save-excursion
    (end-of-line)
    (point)))

(defun test-mode-smie-forward-token ()
  "Move forward by one lexer token. If a change in text triggers it,
such as adding newline, it's called after the change has been made"
  ;; TBM don't hardcode regexes but use smie functions to find parents
  (let ((start (point)))
    (cond
     ((or (progn
            (beginning-of-line)
            (re-search-forward "\\s-*\\w+\\s-*=" (end-of-line-pos) t))
          ;; (while (and (> (point) (point-min))
          ;;             (re-search-forward "\\s-*\\w+\\s-*=\\$$" (end-of-line-pos) t))
          ;;   (forward-line -1))
          )
      (print "here")
      "STRING")
     (t
      (print "or here")
      (smie-default-forward-token))))
  )

(defun test-mode-smie-backward-token ()
  "Move backward by one lexer token."
  (smie-default-backward-token))

(defvar test-mode-keywords
  '("build" "rule"))

(defvar test-mode-font-lock-defaults
  `((
     ;; ;; Highlight comments
     ;; ("\\(#.*\\)" 1 font-lock-comment-face)
     ;; Highlight keywords
     (,(regexp-opt test-mode-keywords 'words) . font-lock-keyword-face)
     ;; Highlight assignments
     ("\\b\\w+\\s-*=" . font-lock-variable-name-face)
     )))

(define-derived-mode test-mode prog-mode "test mode"
  "test mode"
  (setq-local font-lock-defaults test-mode-font-lock-defaults)
  (setq-local comment-start "# ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "#+ *")
  (setq-local comment-use-syntax t)
  ;; (setq-local syntax-propertize-function #'elixir-syntax-propertize-function)
  ;; (setq-local imenu-generic-expression elixir-imenu-generic-expression)

  (smie-setup test-mode-smie-grammar #'test-mode-smie-rules
	      :forward-token #'test-mode-smie-forward-token
	      :backward-token #'test-mode-smie-backward-token))

(provide 'test)
