# english-words

A list of 370,105 English words, by dwyl and its contributors, released into
the public domain (the Unlicense, see `LICENSE.md`).

Upstream: https://github.com/dwyl/english-words, commit
`20f5cc9b3f0ccc8ce45d814c532b7c2031bba31c`. `words_alpha.txt.gz` is its
`words_alpha.txt` (SHA-256
3ed0c94610d8bcf7c11bbb49c56aa49c7234d32b66824df91f554169e572da48), unchanged
but compressed (`gzip -9 -n`).

Nothing the game builds uses it. Streamer mode's tools do:
`tools/text_filter_terms.py` works out where each word it hides may be found
(inside a word, at a word's start, or only as a whole word) from which
English words contain it, and `tools/test_text_filter_terms.py` checks that
the words hidden in English are those expected
(`port/assets/text_filter/english.txt`).
