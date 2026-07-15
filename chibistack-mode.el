;; Copyright (c) 2026 Yuumei-02. All Rights Reserved.
;; See the LICENSE file for more information.

(defvar chibistack-mode-keywords 
   '("proc" "begin" "end" "syscall0" "syscall1" "syscall2" "syscall3" "syscall4" "syscall5" "syscall5" "#include")
   "Chibistack keywords")

(defvar chibistack-mode-types 
   '("int" "uint" "ptr")
   "Chibistack build-in types")

(defvar chibistack-mode-comments
   '("//.*")
   "Chibistack comments")

(defvar chibistack-mode-strings
   '("\"\\([^\"\\]\\|\\\\.\\)*\"")
   "Chibistack strings")

(defvar chibistack-mode-int-literals
   '("\\b[0-9]+\\b")
   "Chibistack int literals")

(defvar chibistack-mode-font-lock-keywords
   (let ((kw-regex (regexp-opt chibistack-mode-keywords 'words))
        (type-regex (regexp-opt chibistack-mode-types 'words)))
   `(
      (,(car chibistack-mode-comments) . font-lock-comment-face)
      (,(car chibistack-mode-strings)   . font-lock-string-face)
      (,type-regex . font-lock-type-face)
      (,kw-regex   . font-lock-keyword-face)
      (,(car chibistack-mode-int-literals) . font-lock-constant-face)
   ))
   "Font lock for chibistack highlights")

(define-derived-mode chibistack-mode prog-mode "Chibistack"
   "Major mode for editing Chibistack files"
   (setq font-lock-defaults '(chibistack-mode-font-lock-keywords)))

(add-to-list 'auto-mode-alist '("\\.csta$" . chibistack-mode))
