from django.urls import path

from . import views

urlpatterns = [
	path('', views.index, name='index'),
	path('submit/',views.submit, name='sub'),
    path('import/',views.impot, name='import'),
    path('login/',views.login, name='login'),
    path('logout/',views.logout_user, name='logout'),
    path('admin/logout/', views.logout_user),
	path('assignment/',views.assign, name='assign'),
    path('results/',views.results, name='results'),
]