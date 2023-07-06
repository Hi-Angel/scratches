(defvar ninja-syntax-table
  (let ((table (make-syntax-table)))
    (modify-syntax-entry ?_ "w" table)
    (modify-syntax-entry ?- "w" table)
    (modify-syntax-entry ?# "<" table)
    (modify-syntax-entry ?\n ">" table)
    (modify-syntax-entry ?\" "\"" table)
    (modify-syntax-entry ?\' "\"" table)
    (modify-syntax-entry ?\{ "(}" table)
    (modify-syntax-entry ?\} "){" table)
    (modify-syntax-entry ?\\ "\\" table)
    (modify-syntax-entry ?$ "$" table)
    table))

(defun ninja-forward-token ()
  (forward-comment (point-max))
  (let ((start (point))
        (end (point)))
    (cond ((looking-at "[^[:space:]#\\$]+")
           (setq end (match-end 0))
           (goto-char end))
          ((looking-at "#")
           (goto-char (line-end-position)))
          ((looking-at "\\$")
           (goto-char (1+ (match-end 0)))
           (ninja-forward-token)
           (setq end (point)))
          (t
            (setq end (1+ (point)))
            (goto-char end)))
    (buffer-substring-no-properties start end)))

(defun ninja-indentation ()
  (save-excursion
    (beginning-of-line)
    (if (looking-at "^ *\\([[:word:]]+\\):")
      (+ (current-indentation) tab-width)
      (current-indentation))))

(defvar ninja-smie-grammar
  (smie-prec2->grammar
    (smie-bnf->prec2
      `((insts (insts "\n" inst) inst)
        (insts inst)
        (inst ("rule" ID)
              ("rule" ID "command" STR)
              ("rule" ID "description" STR)
              ("rule" ID "depfile" STR)
              ("rule" ID "deps" STR)
              ("rule" ID "generator")
              ("rule" ID "restat")
              ("rule" ID "rspfile" STR)
              ("rule" ID "rspfile_content" STR)
              ("build" path ":" path paths)
              ("build" path ":" path paths "|" STR)
              ("build" path ":" path paths "||" STR)
              ("build" path ":" path paths "&&" STR)
              ("build" path ":" path paths "description" STR)
              ("build" path ":" path paths "depfile" STR)
              ("build" path ":" path paths "deps" STR)
              ("build" path ":" path paths "generator")
              ("build" path ":" path paths "restat")
              ("build" path ":" path paths "rspfile" STR)
              ("build" path ":" path paths "rspfile_content" STR))
        (paths (paths path)
               (path))
        (path ID)
        (path STR))
      '((assoc "\n")
        (assoc "|")
        (assoc "||")
        (assoc "&&")
        (assoc ":")
        (assoc "description")
        (assoc "depfile")
        (assoc "deps")
        (assoc "generator")
        (assoc "restat")
        (assoc "rspfile")
        (assoc "rspfile_content")))))

(defun ninja-smie-rules (kind token)
  (pcase (cons kind token)
         (`(:elem . basic) ninja-indentation)
         (`(:elem . comment) nil)
         (`(:elem . string) nil)
         (`(:elem . word) 'rparen)
         (`(:after . "{") (smie-rule-separator kind))
         (`(:before . "}") (smie-rule-separator kind))
         (`(:before . ":") (smie-rule-separator kind))
         (`(:before . ",") (smie-rule-separator kind))
         (`(:before . "|") (smie-rule-separator kind))
         (`(:before . "||") (smie-rule-separator kind))
         (`(:before . "&&") (smie-rule-separator kind))
         (`(:before . "\n") (smie-rule-separator kind))))

(defun ninja-smie-setup ()
  (setq-local comment-start "# ")
  (setq-local comment-end "")
  (smie-setup ninja-smie-grammar #'ninja-forward-token #'ninja-smie-rules))
