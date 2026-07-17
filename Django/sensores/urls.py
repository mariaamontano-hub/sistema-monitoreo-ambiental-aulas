from django.urls import path, include
from rest_framework.routers import DefaultRouter

from .views import (
    MedicionAmbientalViewSet,
    exportar_csv
)

router = DefaultRouter()
router.register(r'mediciones', MedicionAmbientalViewSet)

urlpatterns = [
    path('', include(router.urls)),
    path('exportar-csv/', exportar_csv, name='exportar_csv'),
]