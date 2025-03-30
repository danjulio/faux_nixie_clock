get_em              [runs [PATH]/emscripten/emsdk/emsdk_env.sh]
cd build
emcmake cmake ..    [first time or maybe when things change signficantly or have been deleted]
emmake make -j4
gzip index.html
mv index.html.gz ../../components/web_assets
