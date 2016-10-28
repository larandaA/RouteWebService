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
### Обоснование
Данный сервис предназначен для получения оптимального по времени и по стоимости маршрута.
Будем действовать исходя из предположения, что пользователи не смотрят в последнюю минуту, как им лучше всего добраться из одного города в другой, а некоторые так и вовсе проверяют информацию по нескольку раз. Более того, большинству пользователей нужна не всегда точная информация, а скорее приблизительное значение.
С другой стороны, люди, которые будут изменять данные о рейсах, должны всегда иметь возможность записать какие-либо изменения, которые, возможно, отразятся у пользователей в разное время.
### Потери
Пользователи будут не всегда получать одни и те же данные в случае возникновения проблемы со связью между серверами. Некоторые данные могут быть устаревшими.
### Гарантии
Данный сервис является AP системой, то есть мы жертвуем консистентностью данных для пользователей в случае каких-либо сбоев.
При этом пользователь всегда может отправить запрос на модификацию хранящихся данных и быть уверенным, что его изменения сохранились. 

В случае потери связи после ее восстановления система будет стремиться восстановить консистентность данных.
### Средства достижения AP
Для гарантии AP будет использоваться Cassandra, которая обеспечивает AP. 

Узлы Cassandra будут объеденены в кластер. Поверх каждого узла будет работать отдельный сервер, ничего не знающий о других серверах.
В качестве C++ драйвер будет использоваться Datastax C/C++ driver.

Также для быстрого сохранения/получения данных о запросах (кэширование) будет осуществляться с использованием Redis (redox C++ driver, например).

