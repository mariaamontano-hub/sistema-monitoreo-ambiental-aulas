from django.contrib import admin
from django.urls import path, include
from sensores.views import dashboard

urlpatterns = [
    path('', dashboard, name='inicio'),
    path('admin/', admin.site.urls),
    path('api/', include('sensores.urls')),
]