# SPDX-License-Identifier: GPL-3.0-or-later
"""Tests how check_collection.py reads the logs. No runner or game data."""
import re
import unittest

import check_collection


class CheckCollectionTests(unittest.TestCase):
    def test_reads_the_shop_lines(self):
        shown = re.search(check_collection.UI_SHOP, "[auto:ui] frame 954: shop 0 cursor 3 item 163")
        self.assertEqual(shown.groupdict(), {"shop": "0", "cursor": "3", "item": "163"})
        listed = re.search(check_collection.COLLECTION_SHOP.format(**shown.groupdict()),
                           "[auto:collection] frame 954: shop 0 cursor 3: item 163 white origami (type 2):"
                           " Tina=none Ayane=had+collection,held Lisa=had")
        self.assertEqual(listed["name"], "white origami")
        self.assertEqual(check_collection.owners(listed["owners"]),
                         {"Tina": "none", "Ayane": "had+collection", "Lisa": "had"})

    def test_reads_the_panel_line(self):
        panel = re.search(check_collection.PANEL,
                          "[auto:collection] frame 5: panel: Oreas / Collection: Ayane  |  In room: Ayane;"
                          " lit: heart, present")
        self.assertEqual(panel["text"], "Oreas / Collection: Ayane  |  In room: Ayane")
        self.assertEqual(panel["lit"], "heart, present")

    def test_finds_lines_from_an_index(self):
        lines = ["a 1", "b 2", "a 3"]
        self.assertEqual(check_collection.find(lines, 1, r"a (\d)").group(1), "3")
        self.assertIsNone(check_collection.find(lines, 3, "a"))
        self.assertEqual(check_collection.index_of(lines, "b"), 1)
        self.assertIsNone(check_collection.index_of(lines, "c"))


if __name__ == "__main__":
    unittest.main()
