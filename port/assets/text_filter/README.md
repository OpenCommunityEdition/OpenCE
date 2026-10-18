# Streamer mode's words

The words streamer mode hides (Settings > Network Setup > Streamer Mode; the
`streamer.*` settings in `port/linux/README.md`). The game builds them in
from `port/linux/game/text_filter_terms.inc`, which
`tools/text_filter_terms.py` writes from Surge AI's list and these files:

| File | Contents |
| --- | --- |
| `review.txt` | The review of every base word of Surge AI's list: kept (its group, severity and where it may be found) or dropped, and the spellings dropped. The words are ROT13'd. |
| `allow.txt` | Words never hidden, however a hidden word sits inside them (Scunthorpe), in plain text |
| `english.txt` | The words of the English word list (`port/third_party/english-words`) that the list hides, ROT13'd, which `tools/test_text_filter_terms.py` checks |

`python3 tools/text_filter_terms.py --list` shows the table decoded, and
`--check "some text"` shows what the game would hide in a text.

## Surge AI's list

The words, their categories and their severities come from Surge AI's
profanity list, "The Obscenity List": 1,598 English profanities and their
spellings, each with up to three base words, up to three categories and a
severity that five raters gave it (Mild, Strong or Severe).

Upstream: https://github.com/surge-ai/profanity (gone since; the Internet
Archive's copy of its page of 1 March 2025 shows an MIT `LICENSE`, whose text
it did not keep), copied at https://huggingface.co/datasets/mmathys/profanity
(MIT), revision `c2a17bf3ec64d9b6418ead1cb5f7ef26dbdffccb`: `profanity_en.csv`,
SHA-256 ca4ff7d713a8e6f6ec2616f489cd09f6fb96b8b3af404a207783be58a91f3ede.

The CSV is not in the repository: give `tools/text_filter_terms.py --surge`
its path to write the table again (and `HALO_SURGE_CSV` its path for the
test that checks it).

The groups are made from its categories (`tools/text_filter_terms.py`'s
`GROUPS`), each base word in one, then reviewed (`review.txt`):

| Group | Surge AI's categories |
| --- | --- |
| SLURS | racial / ethnic slurs, sexual orientation / gender, mental disability, physical disability, political |
| SWEARING | sexual anatomy / sexual acts, bodily fluids / excrement, religious offense |
| INSULTS | other / general insult, animal references, physical attributes |

The review adds words of the project's own that their list has not (`term`
lines at its end): sexual words (porn, hentai and the like), words for
sexualised minors, hate phrases and harassment ("kill yourself"), each with a
severity the default levels hide (slurs from mild, swearing from strong).

The table is made from their list, so it carries their notice:

```
MIT License

Copyright (c) Surge AI

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
