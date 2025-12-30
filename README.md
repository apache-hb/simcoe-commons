# Simcoe Commons

Inspired by Apache Commons but for C++
This is mostly a collection of what I've written over the years thats ended up being reusable. But bundled into a repo rather than being copy pasted into every project I write.

## Documentation

* Documentation is published to GitHub Pages [here](https://apache-hb.github.io/simcoe-commons/)

## Using in your own project

### Meson

This project can be added as a git wrap by executing the following command in your project root

```sh
mkdir -p subprojects && echo -e "[wrap-git]\nurl = https://github.com/apache-hb/simcoe-commons.git\nrevision = HEAD\ndepth = 1\n\n[provide]\ndependency_names = simcoe-concurrent, simcoe-defer" >> subprojects/simcoe-commons.wrap
```

And then consumed in your meson project via

```py
simcoe_defer = dependency('simcoe-defer')
```

### Globally

Global installation is available via mesons install command

```sh
git clone https://github.com/apache-hb/simcoe-commons.git
cd simcoe-commons
meson setup builddir --buildtype=release
sudo meson install -C builddir --skip-subprojects
```

After this all modules will be available globally, with pkg-config files generated for libraries with link-time dependencies.
