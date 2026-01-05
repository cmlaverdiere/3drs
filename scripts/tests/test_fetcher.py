"""Tests for fetcher URL resolution (no network calls)."""

import pytest

from image_to_map.fetcher import resolve_wiki_image_url


class TestResolveWikiImageUrl:
    def test_direct_url_passthrough(self):
        url = "https://example.com/image.png"
        assert resolve_wiki_image_url(url) == url

    def test_url_without_file_param(self):
        url = "https://oldschoolrunescape.fandom.com/wiki/Lumbridge"
        assert resolve_wiki_image_url(url) == url

    def test_empty_query_string(self):
        url = "https://example.com/page"
        assert resolve_wiki_image_url(url) == url
