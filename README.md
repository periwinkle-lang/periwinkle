# Барвінок
Українська мова програмування українською :D

## Збірка
### Ubuntu
Встановлення бібліотек потрібних для збірки
```shell
apt install libpcre2-dev
```
#### Реліз версія
```shell
cmake -DCMAKE_BUILD_TYPE=Release && make
```
#### Дебаг версія
```shell
cmake -DCMAKE_BUILD_TYPE=Debug && make
```

## Ліцензія

MIT