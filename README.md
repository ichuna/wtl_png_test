## WTL PNG TEST

* 使用WTL重新实现
* 使用WTL的有点是不依赖vc库中的rc文件
* 使用clang编译以及最新版本的gn构建不会出错
* 使用方法参照[mfc png test](https://github.com/realcome/png_test)

![Image](https://github.com/realcome/wtl_png_test/blob/master/preview.png)

- 项目依赖于(WTL 10(https://sourceforge.net/projects/wtl/files/))[https://sourceforge.net/projects/wtl/files/]
- window sdk 10.0.17134.0
- gn args 中is_clang = false，因为很多代码是vs自动生成的，使用clang规范检查会报错
