from rest_framework import serializers
from .models import MedicionAmbiental


class MedicionAmbientalSerializer(serializers.ModelSerializer):
    class Meta:
        model = MedicionAmbiental
        fields = '__all__'