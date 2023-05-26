Download latest release: [{{ site.github.latest_release.tag_name }}]({{ site.github.latest_release.html_url }})

{% assign assets = site.github.latest_release.assets %}
{% for asset in assets -%}
[{{ asset.name }}]({{ asset.browser_download_url }})
{% endfor %}

![Screenshot](RadNotepad.png)

