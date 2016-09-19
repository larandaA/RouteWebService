# RouteWebService
Позволяет пользователям получать оптимальные маршруты из одной заданной точки в другую, а также добавлять и изменять точки и маршруты, получать список доступных точек и полную информацию о какой-либо конкрутной точке.

-------- Representations -----------------

POINT SHORT REPRESENTATION
<point>
  <name></name>
  <link></link>
</point>

RELATION SHORT REPRESENTATION
<relation>
  <destination>
    <point short representation>
  </destination>
</relation>

POINT FULL REPRESENTATION
<point>
  <name></name>
  <relations>
    <relation short representation>
    <relation short representation>
    ...
    <relation short representation>
  <relations>
</point>

RELATION FULL REPRESENTATION
<relation>
  <source>
    <point short representation>
  </source>
  <destination>
    <point short representation>
  </destination>
  <cost></cost>
  <duration></duration>
</relation>

POINTS COLLECTION REPRESENTATION
<points>
  <point short representation>
  <point short representation>
  ...
  <point short representation>
</points>

-------- Requests Description ------------

GET /points/
Response : points cillection representation

POST /points/ 
Request: <point><name>New point name</name></point>
Response: 
  201 Created or 409 Already exists
  point full representation
  
