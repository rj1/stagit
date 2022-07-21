#!/usr/bin/env python3
import pygments
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments_cache import guess_lexer_for_filename
from pygments.lexers import guess_lexer

from sys import stdin, stderr

filename = stdin.readline().strip()
contents = stdin.read()

lexer=None

try:
    lexer = guess_lexer_for_filename(filename, contents)
except pygments.util.ClassNotFound:
    try:
        lexer = guess_lexer(contents)
    except pygments.util.ClassNotFound:
        pass

if lexer is None:
    from pygments.lexers import TextLexer
    lexer = TextLexer()

FORMAT = HtmlFormatter(
    style='one-dark',
    cssclass='highlight',
    linenos='table',
    lineanchors='loc',
    anchorlinenos=True)

print('<div id="blob">')
print(highlight(contents, lexer, FORMAT))
print('</div>')

print(f"processing {filename} with {lexer}", file=stderr)
