# obfuscator

这是一个用 `c++` 编写的 `c++` 代码混淆器

## 使用方法
>
> 克隆代码并编译

- **Linux**

```bash
./setup.sh
obfuscator_cpp <file_path> <out_path>
```

## 可选参数

- `-x string string` 接收两套编码集，先按照词混合，在按照行混合
- `-h string` 接受一个性签名，并把它放在输出文件的头部
- `-s string` 自定义编码集，用空格隔开
- `-M number` 自定义每行输最大输出字符数
- `-m number` 自定义每行输出个数
- `-l` 使用每行代码作为token
- `-t` 将混淆代码翻译为原代码
- `-n` 不使用混淆编码
