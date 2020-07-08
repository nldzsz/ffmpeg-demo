package com.example.demo.Common;

public class ADAssetsManager {

    /** 对AssetManager的一个封装
     *      Android 系统为每个新设计的程序提供了/assets目录，这个目录保存的文件可以打包在程序里。/res 和/assets的不同点是，android不为/assets下的文件生成ID。
     * 如果使用/assets下的文件，需要指定文件的路径和文件名。
     *      ContextImpl类的成员函数getResources返回的是一个Resources对象，有了这个Resources对象之后，我们就可以通过资源ID来访问那些被编译过的应用程序资源了。
     * ContextImpl类的成员函数getAssets返回的是一个AssetManager对象，有了这个AssetManager对象之后，我们就可以通过文件名来访问那些被编译过或者没有被编译过
     * 的应用程序资源文件了。事实上，Resources类也是通过AssetManager类来访问那些被编译过的应用程序资源文件的，不过在访问之前，它会先根据资源ID查找得到对应的资
     * 源文件名。
     *      Android应用程序除了要访问自己的资源之外，还需要访问系统的资源。系统的资源打包在/system/framework/framework-res.apk文件中，它在应用程序进程中是通
     * 过一个单独的Resources对象和一个单独的AssetManager对象来管理的。这个单独的Resources对象就保存在Resources类的静态成员变量mSystem中，我们可以通过Resources
     * 类的静态成员函数getSystem就可以获得这个Resources对象，而这个单独的AssetManager对象就保存在AssetManager类的静态成员变量sSystem中，我们可以通过
     * AssetManager类的静态成员函数getSystem同样可以获得这个AssetManager对象。
     * */

    /** 放在AssetManager中的单个文件大小不能超过1M，否则会报错。但是如下后缀名没有这个限制
     * static const char* kNoCompressExt[] = {
     * 　　".jpg", ".jpeg", ".png", ".gif",
     * 　　".wav", ".mp2", ".mp3", ".ogg", ".aac",
     * 　　".mpg", ".mpeg", ".mid", ".midi", ".smf", ".jet",
     * 　　".rtttl", ".imy", ".xmf", ".mp4", ".m4a",
     * 　　".m4v", ".3gp", ".3gpp", ".3g2", ".3gpp2",
     * 　　".amr", ".awb", ".wma", ".wmv"
     * 　　};
     * */
}
