# RouteWebService
Позволяет пользователям получать оптимальные маршруты из одной заданной точки в другую, а также добавлять и изменять точки и маршруты, получать список доступных точек и полную информацию о какой-либо конкрутной точке.
## REST
Здесь подходит REST, так как передаются представления ресурсов, могут кэшироваться ответы сервера. 
Однако REST с некоторыми поправками, так как не все ресурсы идентифицируются, а также сообщения не являются самоописывающимися.
### Representations
#### Point short representation
```xml
<point>
  <name></name>
  <link></link>
</point>
```
#### Relation short representation
```xml
<relation>
  <destination>
    <point-short-representation/>
  </destination>
  <link></link>
</relation>
```
#### Point full representation
```xml
<point>
  <name></name>
  <relations>
    <relation-short-representation/>
    <relation-short-representation/>
    ...
    <relation-short-representation/>
  <relations>
</point>
```
#### Relation full representation
```xml
<relation>
  <source>
    <point-short-representation/>
  </source>
  <destination>
    <point-short-representation/>
  </destination>
  <cost></cost>
  <duration></duration>
</relation>
```
#### Points collection representation
```xml
<points>
  <point-short-representation/>
  <point-short-representation/>
  ...
  <point-short-representation/>
</points>
```
#### Route representation
```xml
<route>
  <source>
    <point-short-representation/>
  </source>
  <destination>
    <point-short-representation/>
  </destination>
  <cost></cost>
  <duration></duration>
  <relations>
    <relation-short-representation/>
    <relation-short-representation/>
    ...
    <relation-short-representation/>
  </relations>
</route>
```
### Requests description
#### Getting collection of points
Method
```
GET /points/
```
Response
```xml
<points-collection-representation/>
```
#### Adding new point
Method
```
POST /points/ 
```
Request
```xml
<point>
	<name>New point name</name>
</point>
```
Response if created
```xml
201 Created; Location: /points/point_id
<point-short-representation/>
```
Response if exists
```xml
409 Already exists; Location: /points/point_id
<point-short-representation/>
```
#### Getting point
Method
```
GET /points/point_id 
```
Response if successful
```xml
Location: /points/point_id 
<point-full-representation/>
```
Response if failed
```
404 Not found
```
#### Deleting point
Method
```
DELETE /points/point_id
```
Response if successfull
```
404 Not found
```
Response if failed
```xml
<point-full-representation/>
```
#### Modifying point
Method
```
PUT /points/point_id
```
Request
```xml
<point>
	<name>Point new name</name>
</point>
```
Response if succesfull
```xml
Location: /points/point_id
<point-short-representation/>
```
Response if failed
```
404 Not found
```
#### Getting relation
Method
```
GET /relations/relation_id
```
Response if successful
```xml
Location: /points/relation_id
<relation-full-representation/>
```
Response if failed
```
404 Not found
```
#### Deleting relation
Method
```
DELETE /relations/relation_id
```
Response if successful
```xml
<relation-full-representation/>
```
Response if failed
```
404 Not found
```
#### Modifying relation
Method
```
PUT /relations/relation_id
```
Request
```xml
<relation>
	<cost></cost>
	<duration></duration>
</relation>
 ```
Response if successful
```xml
Location: /points/relation_id
<relation-full-representation/>
```
Response if failed
```
404 Not found
```
#### Adding new relation
Method
```
POST /relations/
```
Request
```xml
<relation-full-representation/>
```
Response if created
```xml
201 Created; Location: /points/relation_id
<relation-full-representation/>
```
Response if exists 
```xml
409 Already exists; Location: /points/relation_id
<relation-full-representation/>
```
#### Getting route
Searches by link if it not empty, and by name otherwise.

Method
```
GET /route/
```
Request
```xml
<route>
	<source>
		<point-short-representation/>
	</source>
	<destination>
		<point-short-representation/>
	</destination>
</route>
```
Response if successful
```xml
<route-representation/>
```
Response if failed
```
404 Not found
```

## CAP
### Проблема
Данные, хранящиеся в базе данных, представляют собой граф. При этом встает проблема обеспечения невозможности добавления ребра, инцидентного несуществующей вершине, и прочих подобных ситуаций, разрешение которых является достаточно сложной задачей.
### Mongo
Для хранения данных было решено использовать Mongo db, которая обеспечивает CP в рамках одного документа. Это определяет, какая стратегия будет выбрана при падении узла-мастера. Однако это не решает проблемы добавления неверных данных.

### Консистентность графа
Перед каждой операцией записи должна проводиться проверка на возможность проведения этой операции. Проверка вместе с записью (либо отказом от нее) должна быть в одной транзакции, то есть мы должны быть уверены, что между проверкой и собственно записью не произошло никаких изменений в графе, которые могут привести к конфликтности данных.

Так как подобная атомарность действия должна быть реализована на уровне всей базы данных, то было принято решение обеспечивать ее с помощью служебной таблицы в базе данных. При этом блокирующим объектом будет являться один документ, консистентность которого Mongo сможет обеспечить. 

Так как необходимо обеспечивать возможность параллельной записи в базу данных, то документ должен блокировать только ту часть графа, которая может повлиять на результат выборки данных. Так как ребра почти полностью определяются вершинами, которым они инцидентны, то достаточно блокировать только вершины, с которыми связана проверка. Чтобы избежать dead locks и race conditions, информация о заблокированных вершинах должна храниться в одном документе, а не нескольких.

Тогда операция записи может выглядеть следующим образом:
1. Получение блокирующего документа.
2. Проверка на наличие в нем хотя бы одной из требующихся вершин.
3. При положительном результате вернуться на 1 шаг.
4. Добавить в документ требующиеся вершины.
5. Провести необходимую проверку на возможность записи.
6. Запись либо отказ от записи.
7. Удаление из блокирующего документа освободившиеся вершины.

Пункты данного алгоритма могут измениться при попытке релизации его на практике.

## Replica set

### Primary
В файл конфигурации /etc/mongodb.conf процесса-демона mongod была добавлена строка:
```
replSet = rws_rs
```
Данная строка указывает, к какой реплике относится данный экземпляр MongoDB. Для того, чтобы внесенные изменения повлияли на настройку бд, она была перезапущена.

### Secondary
Были запущены два дополнительных процесса MongoDB, которые в последствии станут secondary участниками реплики. Процессы были запущены следующими командами:
```
$ sudo mongod --fork --dbpath /var/lib/mongodb2 --logpath /var/log/mongodb/mongodb2.log --bind_ip 127.0.0.1 --port 27019 --replSet "rws_rs"
$ sudo mongod --fork --dbpath /var/lib/mongodb3 --logpath /var/log/mongodb/mongodb3.log --bind_ip 127.0.0.1 --port 27021 --replSet "rws_rs"
```
Таким образом, удалось запустить несколько экземпляров бд, указав им разные файлы логгирования, разные пути к директориям размещения файлов баз данных, а также разные порты, по которым они будут работать.

### Initiation
Для инициации реплики была запущена mongo shell, подключенная к будущему primary экземпляру MongoDB, то есть была просто выполнена команда:
```
$ mongo
```
Далее была выполнена следующая команда:
```
> rs.initiate({_id:"rws_rs", members:[{_id: 0, host: "127.0.0.1:27017"}]})
```
Для добавления новых участников в реплику, которые впоследствии станут secondary, были выполнены следующие команды:
```
PRIMARY> rs.add("127.0.0.1:27019")
PRIMARY> rs.add("127.0.0.1:27021")
```
Об успехе проведенной работы нам сообщит команда:
```
PRIMARY> rs.status()
```
Результат ее выполнения:
```json
{
        "set" : "rws_rs",
        "date" : ISODate("2017-01-15T21:15:20Z"),
        "myState" : 1,
        "members" : [
                {
                        "_id" : 0,
                        "name" : "127.0.0.1:27017",
                        "health" : 1,
                        "state" : 1,
                        "stateStr" : "PRIMARY",
                        "optime" : {
                                "t" : 1484514878000,
                                "i" : 1
                        },
                        "optimeDate" : ISODate("2017-01-15T21:14:38Z"),
                        "self" : true
                },
                {
                        "_id" : 1,
                        "name" : "127.0.0.1:27019",
                        "health" : 1,
                        "state" : 2,
                        "stateStr" : "SECONDARY",
                        "uptime" : 52,
                        "optime" : {
                                "t" : 1484514878000,
                                "i" : 1
                        },
                        "optimeDate" : ISODate("2017-01-15T21:14:38Z"),
                        "lastHeartbeat" : ISODate("2017-01-15T21:15:18Z"),
                        "pingMs" : 0
                },
                {
                        "_id" : 2,
                        "name" : "127.0.0.1:27021",
                        "health" : 1,
                        "state" : 2,
                        "stateStr" : "SECONDARY",
                        "uptime" : 42,
                        "optime" : {
                                "t" : 1484514878000,
                                "i" : 1
                        },
                        "optimeDate" : ISODate("2017-01-15T21:14:38Z"),
                        "lastHeartbeat" : ISODate("2017-01-15T21:15:18Z"),
                        "pingMs" : 5018480
                }
        ],
        "ok" : 1
}
```
