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
### Neo4j
Для хранения графа была выбрана графовая база данных Neo4j. Согласно официальному сайту, данная субд является ACID, то есть операции записи являются транзакционными. Кластер настраивается таким образом, что среди узлов выделяется мастер, только который может записывать и копировать информацию на другие узлы. Читать данные можно со всех узлов. Так как запросов на запись к данному сервису будет гораздо меньше, чем запросов на чтение, то подобная архитектура позволит распределить нагрузку среди узлов боле-менее равномерно.

При этом прри падении мастера в кластере автоматически выбирается новый мастер. Однако неизвестно, что произойдет при распадении кластера на две и более групп узлов. Это будет выяснено уже на практике.
### Итого
Поверх каждого узла будет работать сервер, обрабатывающий запросы клиентов и отправляющий запросы к своему узлу Neo4j.
Таким образом, данная система в рамках CAP теоремы предположительно является CA системой.
