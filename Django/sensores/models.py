from django.db import models


class MedicionAmbiental(models.Model):
    nodo = models.CharField(max_length=50, default="nodo_01")
    aula = models.CharField(max_length=100, default="Aula de prueba")

    temperatura = models.FloatField()
    humedad = models.FloatField()
    co2 = models.IntegerField()
    iluminacion = models.FloatField()
    ruido = models.FloatField()
    movimiento = models.BooleanField(default=False)

    fecha_hora = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"{self.nodo} - {self.aula} - {self.fecha_hora}"